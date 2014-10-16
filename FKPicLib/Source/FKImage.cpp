//-------------------------------------------------------------------------
#include "../Include/FKImage.h"
#include "../Include/FKIOFile.h"
#include "../Include/FKMemFile.h"
#include "../Include/FKImageBmp.h"
#include "../Include/FKImageJpeg.h"
#include "../Include/FKImagePng.h"
#include "../Include/FKImageFKP.h"
//-------------------------------------------------------------------------
CFKImage::CFKImage( DWORD p_dwImageType )
{
	_StartUp( p_dwImageType );
}
//-------------------------------------------------------------------------
CFKImage::~CFKImage()
{
	DestroyFrames();
	Destroy();
}
//-------------------------------------------------------------------------
CFKImage& CFKImage::operator = ( const CFKImage& p_tagOther )
{
	if( this != &p_tagOther )
		Copy( p_tagOther );
	return *this;
}
//-------------------------------------------------------------------------
void* CFKImage::Create( DWORD p_dwWidth, DWORD p_dwHeight, DWORD p_dwBpp, DWORD p_dwImageType )
{
	// 如果已有Image信息，直接销毁
	if( !Destroy() )
		return NULL;

	if( (p_dwWidth == 0) || (p_dwHeight == 0) )
		return NULL;

	//  修正每像素位数
	if( p_dwBpp <= 1 )
		p_dwBpp = 1;
	else if( p_dwBpp <= 4 )
		p_dwBpp = 4;
	else if( p_dwBpp <= 8 )
		p_dwBpp = 8;
	else
		p_dwBpp = 24;

	// 最小内存需求检查
	if( ( ( p_dwWidth * p_dwHeight * p_dwBpp ) >> 3 > IMAGE_MAX_MEMORY ) ||
		( ( p_dwWidth * p_dwHeight * p_dwBpp ) / p_dwBpp != ( p_dwWidth * p_dwHeight ) ) )
	{
		return NULL;
	}

	// 设置调色板位大小
	switch ( p_dwBpp )
	{
	case 1:
		m_tagHead.biClrUsed		= 2;
		break;
	case 4:
		m_tagHead.biClrUsed		= 16;
		break;
	case 8:
		m_tagHead.biClrUsed		= 256;
		break;
	default:
		m_tagHead.biClrUsed		= 0;
		break;
	}

	// 设置常规图片信息
	m_tagInfo.m_dwEffWidth		= (((( p_dwBpp * p_dwWidth ) + 31 ) / 32 ) * 4 );
	m_tagInfo.m_dwType			= p_dwImageType;

	// 初始化BITMAP头信息
	m_tagHead.biSize			= sizeof( SBitmapInfoHeader );
	m_tagHead.biWidth			= p_dwWidth;
	m_tagHead.biHeight			= p_dwHeight;
	m_tagHead.biPlanes			= 1;
	m_tagHead.biBitCount		= (WORD)p_dwBpp;
	m_tagHead.biCompression		= eBI_RGB;
	m_tagHead.biSizeImage		= m_tagInfo.m_dwEffWidth * p_dwHeight;

	m_pDib = malloc( GetSize() );
	if( m_pDib == NULL )
		return NULL;

	// 清除调色板
	SRGBQuad* pRGBGuad = GetPalette();
	if( pRGBGuad )
		memset( pRGBGuad, 0, GetPaletteSize() );

#if USE_FK_SELECTION
	if( m_pSelection )
		DeleteSelection();
#endif

#if USE_FK_ALPHA
	if( m_pAlpha )
		DeleteAlpha();
#endif

	SBitmapInfoHeader* lpBI;
	lpBI = (SBitmapInfoHeader*)m_pDib;
	*lpBI = m_tagHead;

	m_tagInfo.m_byImage		= GetBits();

	return m_pDib;
}
//-------------------------------------------------------------------------
bool CFKImage::Destroy()
{
	if( m_tagInfo.m_pGhost == NULL )
	{
		if( m_ppLayers )
		{
			for( long n = 0; n < m_tagInfo.m_lNumLayers; ++n )
			{
				delete m_ppLayers[n];
			}
			delete [] m_ppLayers;
			m_ppLayers = NULL;
			m_tagInfo.m_lNumLayers = 0;
		}
		if( m_pSelection )
		{
			free( m_pSelection );
			m_pSelection = NULL;
		}
		if( m_pAlpha )
		{
			free( m_pAlpha );
			m_pAlpha = NULL;
		}
		if( m_pDib )
		{
			free( m_pDib );
			m_pDib = NULL;
		}

		return true;
	}
	return false;
}
//-------------------------------------------------------------------------
bool CFKImage::DestroyFrames()
{
	if( m_tagInfo.m_pGhost == NULL )
	{
		if( m_ppFrames )
		{
			for( long n = 0; n < m_tagInfo.m_lNumFrames; ++n )
			{
				delete m_ppFrames[n];
			}
			delete [] m_ppFrames;
			m_ppFrames = NULL;
			m_tagInfo.m_lNumFrames = 0;
		}
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------
void CFKImage::Copy( const CFKImage& p_tagSrc, bool p_bCopyPixels, 
						 bool p_bCopySelection, bool p_bCopyAlpha )
{
	if( p_tagSrc.m_tagInfo.m_pGhost )
	{
		_Ghost( &p_tagSrc );
		return;
	}

	// 拷贝部分信息
	memcpy( &m_tagHead, &p_tagSrc.m_tagHead, sizeof( SBitmapInfoHeader ) );
	memcpy( &m_tagInfo, &p_tagSrc.m_tagInfo, sizeof( SFKImageInfo ) );

	// 创建图像
	Create( p_tagSrc.GetWidth(), p_tagSrc.GetHeight(), p_tagSrc.GetBpp(), p_tagSrc.GetType() );

	// 拷贝其他信息和调色板（至少要拷贝调色板）
	if( p_bCopyPixels && m_pDib && p_tagSrc.m_pDib )
	{
		memcpy( m_pDib, p_tagSrc.m_pDib, GetSize() );
	}
	else
	{
		SetPalette( p_tagSrc.GetPalette() );
	}

	long lSize = m_tagHead.biWidth * m_tagHead.biHeight;
	// 拷贝选择区
	if( p_bCopySelection && p_tagSrc.m_pSelection )
	{
		if( m_pSelection )
		{
			free( m_pSelection );
		}
		m_pSelection = ( BYTE* )malloc( lSize );
		memcpy( m_pSelection, p_tagSrc.m_pSelection, lSize );
	}

	// 拷贝alpha通道
	if( p_bCopyAlpha && p_tagSrc.m_pAlpha )
	{
		if( m_pAlpha )
		{
			free( m_pAlpha );
		}
		m_pAlpha = ( BYTE* )malloc( lSize );
		memcpy( m_pAlpha, p_tagSrc.m_pAlpha, lSize );
	}
}
//-------------------------------------------------------------------------
bool CFKImage::Load( const TCHAR* p_szFileName, DWORD p_dwImageType )
{
	bool bOk = false;
	if( p_dwImageType != 0 )
	{
		FILE* pFile = NULL;
#ifdef _WIN32
		if( (pFile = _tfopen( p_szFileName, _T("rb"))) == NULL )
			return false;
#else
		if( ( pFile = fopen( p_szFileName, "rb")) == NULL )
			return false;
#endif
		bOk = Decode( pFile, p_dwImageType );
		fclose( pFile );
		if( bOk )
			return bOk;
	}

	char szError[256];
	strcpy( szError, m_tagInfo.m_szLastError );
	FILE* pFile = NULL;

#ifdef _WIN32
	if( (pFile = _tfopen( p_szFileName, _T("rb"))) == NULL )
		return false;
#else
	if( ( pFile = fopen( p_szFileName, "rb")) == NULL )
		return false;
#endif

	bOk = Decode( pFile, eFormats_Unknown );
	fclose( pFile );

	if( !bOk && p_dwImageType > 0 )
	{
		strcpy( m_tagInfo.m_szLastError, szError );
	}
	return bOk;
}
//-------------------------------------------------------------------------
bool CFKImage::Decode( FILE* p_pFile, DWORD p_dwImageType )
{
	CFKIOFile file( p_pFile );
	return Decode( &file, p_dwImageType );
}
//-------------------------------------------------------------------------
bool CFKImage::Decode( IFKFile* p_pFile, DWORD p_dwImageType )
{
	if( p_pFile == NULL )
	{
		strcpy( m_tagInfo.m_szLastError, ERR_NO_FILE );
		return false;
	}

	if( p_dwImageType == eFormats_Unknown )
	{
		DWORD dwPos = p_pFile->Tell();
		{
			CFKImageBMP newImage;
			newImage._CopyInfo(*this);
			if( newImage.Decode(p_pFile) )
			{
				Transfer( newImage );
				return true;
			}
			else
			{
				p_pFile->Seek( dwPos, SEEK_SET );
			}
		}
		{
			CFKImageJPG newImage;
			newImage._CopyInfo(*this);
			if( newImage.Decode(p_pFile) )
			{
				Transfer( newImage );
				return true;
			}
			else
			{
				p_pFile->Seek( dwPos, SEEK_SET );
			}
		}
		{
			CFKImagePNG newImage;
			newImage._CopyInfo(*this);
			if( newImage.Decode(p_pFile) )
			{
				Transfer( newImage );
				return true;
			}
			else
			{
				p_pFile->Seek( dwPos, SEEK_SET );
			}
		}
		{
			CFKImageFKP newImage;
			newImage._CopyInfo(*this);
			if( newImage.Decode(p_pFile) )
			{
				Transfer( newImage );
				return true;
			}
			else
			{
				p_pFile->Seek( dwPos, SEEK_SET );
			}
		}
	}

	if( p_dwImageType == eFormats_BMP )
	{
		CFKImageBMP newImage;
		newImage._CopyInfo(*this);
		if( newImage.Decode(p_pFile) )
		{
			Transfer( newImage );
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}

	if( p_dwImageType == eFormats_JPG )
	{
		CFKImageJPG newImage;
		newImage._CopyInfo(*this);
		if( newImage.Decode(p_pFile) )
		{
			Transfer( newImage );
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}

	if( p_dwImageType == eFormats_PNG)
	{
		CFKImagePNG newImage;
		newImage._CopyInfo(*this);
		if( newImage.Decode(p_pFile) )
		{
			Transfer( newImage );
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}

	if( p_dwImageType == eFormats_FKP )
	{
		CFKImageFKP newImage;
		newImage._CopyInfo(*this);
		if( newImage.Decode(p_pFile) )
		{
			Transfer( newImage );
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}

	strcpy( m_tagInfo.m_szLastError,"Decode: Unknown or wrong format");
	return false;
}
//-------------------------------------------------------------------------
bool CFKImage::Decode( BYTE* p_szBuffer, DWORD p_dwSize, DWORD p_dwImageType )
{
	CFKMemFile file( p_szBuffer, p_dwSize );
	return Decode( &file, p_dwImageType );
}
//-------------------------------------------------------------------------
bool CFKImage::Transfer( CFKImage &p_From, bool p_bTransferFrames )
{
	if( !Destroy() )
		return false;

	memcpy( &m_tagHead, &p_From.m_tagHead, sizeof( SBitmapInfoHeader ) );
	memcpy( &m_tagInfo, &p_From.m_tagInfo, sizeof( SFKImageInfo ) );

	m_pDib				= p_From.m_pDib;
	m_pSelection		= p_From.m_pSelection;
	m_pAlpha			= p_From.m_pAlpha;
	m_ppLayers			= p_From.m_ppLayers;

	memset( &p_From.m_tagHead, 0, sizeof(SBitmapInfoHeader) );
	memset( &p_From.m_tagInfo, 0, sizeof(SFKImageInfo) );
	p_From.m_pDib = p_From.m_pSelection = p_From.m_pAlpha = NULL;
	p_From.m_ppLayers = NULL;

	if( p_bTransferFrames )
	{
		DestroyFrames();
		m_ppFrames = p_From.m_ppFrames;
		p_From.m_ppFrames = NULL;
	}
	return true;
}
//-------------------------------------------------------------------------
bool CFKImage::Save( const TCHAR* p_szFileName, DWORD p_dwImageType )
{
	FILE* pFile = NULL;
#ifdef _WIN32
	if( ( pFile = _tfopen( p_szFileName, _T("wb"))) == NULL )
		return false;
#else
	if( ( pFile = fopen( p_szFileName, "wb")) == NULL )
		return false;
#endif

	bool bOK = Encode( pFile, p_dwImageType );
	fclose( pFile );
	return bOK;
}
//-------------------------------------------------------------------------
bool CFKImage::Encode( FILE* p_pFile, DWORD p_dwImageType )
{
	CFKIOFile file( p_pFile );
	return Encode( &file, p_dwImageType );
}
//-------------------------------------------------------------------------
bool CFKImage::Encode( IFKFile* p_pFile, DWORD p_dwImageType )
{
	if( p_dwImageType == eFormats_BMP )
	{
		CFKImageBMP newImage;
		newImage._Ghost(this);
		if( newImage.Encode(p_pFile) )
		{
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}
	if( p_dwImageType == eFormats_JPG )
	{
		CFKImageJPG newImage;
		newImage._Ghost(this);
		if( newImage.Encode(p_pFile) )
		{
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}
	if( p_dwImageType == eFormats_PNG )
	{
		CFKImagePNG newImage;
		newImage._Ghost(this);
		if( newImage.Encode(p_pFile) )
		{
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}

	if( p_dwImageType == eFormats_FKP )
	{
		CFKImageFKP newImage;
		newImage._Ghost(this);
		if( newImage.Encode(p_pFile) )
		{
			return true;
		}
		else
		{
			strcpy( m_tagInfo.m_szLastError, newImage.GetLastError() );
			return false;
		}
	}

	strcpy( m_tagInfo.m_szLastError,"Encode: Unknown format" );
	return false;
}
//-------------------------------------------------------------------------
bool CFKImage::Encode( BYTE*& p_Buffer, long& p_dwSize, DWORD p_dwImageType )
{
	if( p_Buffer != NULL )
	{
		strcpy( m_tagInfo.m_szLastError,"the buffer must be empty" );
		return false;
	}

	CFKMemFile file;
	file.Open();
	if( Encode(&file, p_dwImageType) )
	{
		p_Buffer = file.GetBuffer();
		p_dwSize = file.Size();
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------
long CFKImage::GetXDpi() const
{
	return m_tagInfo.m_lXDpi;
}
//-------------------------------------------------------------------------
long CFKImage::GetYDpi() const
{
	return m_tagInfo.m_lYDpi;
}
//-------------------------------------------------------------------------
void CFKImage::SetXDpi( long p_lDpi )
{
	if( p_lDpi <= 0 )
		p_lDpi = static_cast<long>( DEFAULT_DPI );

	m_tagInfo.m_lXDpi				= p_lDpi;
	m_tagHead.biXPelsPerMeter		= static_cast<long>( floor( p_lDpi * 10000.0 / 254.0 + 0.5 ) );
	if( m_pDib )
		((SBitmapInfoHeader*)m_pDib )->biXPelsPerMeter	= m_tagHead.biXPelsPerMeter;
}
//-------------------------------------------------------------------------
void CFKImage::SetYDpi( long p_lDpi )
{
	if( p_lDpi <= 0 )
		p_lDpi = static_cast<long>( DEFAULT_DPI );

	m_tagInfo.m_lYDpi				= p_lDpi;
	m_tagHead.biYPelsPerMeter		= static_cast<long>( floor( p_lDpi * 10000.0 / 254.0 + 0.5 ) );
	if( m_pDib )
		((SBitmapInfoHeader*)m_pDib )->biYPelsPerMeter	= m_tagHead.biYPelsPerMeter;
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetClrImportant() const
{
	return m_tagHead.biClrImportant;
}
//-------------------------------------------------------------------------
void CFKImage::SetClrImportant( DWORD p_dwColors )
{
	if( p_dwColors == 0 || p_dwColors > 256 )
	{
		m_tagHead.biClrImportant = 0;
		return ;
	}
	switch ( m_tagHead.biBitCount )
	{
	case 1:
		m_tagHead.biClrImportant = min( p_dwColors, 2 );
		break;
	case 4:
		m_tagHead.biClrImportant = min( p_dwColors, 6 );
		break;
	case 8:
		m_tagHead.biClrImportant = p_dwColors;
		break;
	default:
		break;
	}
}
//-------------------------------------------------------------------------
SRGBQuad CFKImage::GetTransColor()
{
	if( m_tagHead.biBitCount < 24 && m_tagInfo.m_lBkgndIndex >= 0 )
		return GetPaletteColor( (BYTE) m_tagInfo.m_lBkgndIndex );
	return m_tagInfo.m_tagBkgndColor;
}
//-------------------------------------------------------------------------
long CFKImage::GetSize()
{
	// 头自身大小 + 图片大小 + 调色板大小
	return m_tagHead.biSize + m_tagHead.biSizeImage + GetPaletteSize();
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetPaletteSize()
{
	return m_tagHead.biClrUsed * sizeof( SRGBQuad );
}
//-------------------------------------------------------------------------
SRGBQuad* CFKImage::GetPalette() const
{
	if( m_pDib == NULL )
		return NULL;
	if( m_tagHead.biClrUsed == 0 )
		return NULL;

	return ( SRGBQuad* )( (BYTE*)m_pDib + sizeof( SBitmapInfoHeader ) );
}
//-------------------------------------------------------------------------
void CFKImage::SetGrayPalette()
{
	if(( m_pDib == NULL ) || ( m_tagHead.biClrUsed == 0 ))
		return;
	SRGBQuad* pQuad = GetPalette();
	for( DWORD dwN = 0; dwN < m_tagHead.biClrUsed; ++dwN )
	{
		pQuad[dwN].rgbBlue = pQuad[dwN].rgbGreen = pQuad[dwN].rgbRed = ( BYTE )( dwN *( 255 / ( m_tagHead.biClrUsed - 1 )));
	}
}
//-------------------------------------------------------------------------
void CFKImage::SetPalette( DWORD p_dwN, BYTE* p_byRed, BYTE* p_byGreen, BYTE* p_byBlue )
{
	if(( p_byRed == NULL ) || ( m_pDib == NULL ) || ( m_tagHead.biClrUsed == 0 ))
		return;
	if( p_byGreen == NULL )
		p_byGreen = p_byRed;
	if( p_byBlue == NULL )
		p_byBlue= p_byGreen;
	SRGBQuad* pQuad = GetPalette();
	DWORD dwM = min( p_dwN, m_tagHead.biClrUsed );
	for( DWORD i = 0; i < dwM; ++i )
	{
		pQuad[i].rgbRed			= p_byRed[i];
		pQuad[i].rgbGreen		= p_byGreen[i];
		pQuad[i].rgbBlue		= p_byBlue[i];
	}
	m_tagInfo.m_bLastCIsValid	= false;
}
//-------------------------------------------------------------------------
void CFKImage::SetPalette( SRGBQuad* p_pPalette,DWORD p_dwColors )
{
	if( p_pPalette == NULL )
		return;
	if( m_pDib == NULL )
		return;
	if( m_tagHead.biClrUsed == 0 )
		return;

	memcpy( GetPalette(), p_pPalette, min(GetPaletteSize(), p_dwColors * sizeof(SRGBQuad)) );
	m_tagInfo.m_bLastCIsValid = false;
}
//-------------------------------------------------------------------------
void CFKImage::SetPalette( SRGBColor* p_pRgb,DWORD p_dwColors )
{
	if( p_pRgb == NULL )
		return;
	if( m_pDib == NULL )
		return;
	if( m_tagHead.biClrUsed == 0 )
		return;

	SRGBQuad* pPal = GetPalette();
	DWORD dwM = min( p_dwColors, m_tagHead.biClrUsed );
	for( DWORD i = 0; i < dwM; ++i )
	{
		pPal[i].rgbRed		= p_pRgb[i].r;
		pPal[i].rgbGreen	= p_pRgb[i].g;
		pPal[i].rgbBlue		= p_pRgb[i].b;
	}
	m_tagInfo.m_bLastCIsValid = false;
}
//-------------------------------------------------------------------------
SRGBQuad CFKImage::GetPaletteColor( BYTE p_byIndex )
{
	SRGBQuad rgb = { 0, 0, 0, 0 };
	if( m_pDib && m_tagHead.biClrUsed )
	{
		BYTE* pDst = (BYTE*)( m_pDib ) + sizeof( SBitmapInfoHeader );
		if( p_byIndex < m_tagHead.biClrUsed )
		{
			long lDx = p_byIndex * sizeof( SRGBQuad );
			rgb.rgbBlue		= pDst[lDx++];
			rgb.rgbGreen	= pDst[lDx++];
			rgb.rgbRed		= pDst[lDx++];
			rgb.rgbReserved	= pDst[lDx];
		}
	}
	return rgb;
}
//-------------------------------------------------------------------------
bool CFKImage::GetPaletteColor( BYTE p_byIndex, BYTE* p_pRed, BYTE* p_pGreen, BYTE* p_pBlue )
{
	SRGBQuad* pPal = GetPalette();
	if( pPal )
	{
		*p_pRed			= pPal[p_byIndex].rgbRed;
		*p_pGreen		= pPal[p_byIndex].rgbGreen;
		*p_pBlue		= pPal[p_byIndex].rgbBlue;
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------
void CFKImage::SetPaletteColor( BYTE p_byIndx, SRGBQuad p_tagRGB )
{
	if(( m_pDib )&&( m_tagHead.biClrUsed ))
	{
		BYTE* pDst = ( BYTE* )( m_pDib ) + sizeof( SBitmapInfoHeader );
		if( p_byIndx < m_tagHead.biClrUsed )
		{
			long lIndx = p_byIndx * sizeof(SRGBQuad);
			pDst[lIndx++]	= ( BYTE )p_tagRGB.rgbBlue;
			pDst[lIndx++]	= ( BYTE )p_tagRGB.rgbGreen;
			pDst[lIndx++]	= ( BYTE )p_tagRGB.rgbRed;
			pDst[lIndx]		= ( BYTE )p_tagRGB.rgbReserved;
			m_tagInfo.m_bLastCIsValid = false;
		}
	}
}
//-------------------------------------------------------------------------
void CFKImage::SetPaletteColor( BYTE p_byIndx, BYTE p_byRed, BYTE p_byGreen, BYTE p_byBlue, BYTE p_byAlpha )
{
	if(( m_pDib )&&( m_tagHead.biClrUsed ))
	{
		BYTE* pDst = ( BYTE* )( m_pDib ) + sizeof( SBitmapInfoHeader );
		if( p_byIndx < m_tagHead.biClrUsed )
		{
			long lIndx = p_byIndx * sizeof(SRGBQuad);
			pDst[lIndx++]	= ( BYTE )p_byBlue;
			pDst[lIndx++]	= ( BYTE )p_byGreen;
			pDst[lIndx++]	= ( BYTE )p_byRed;
			pDst[lIndx]		= ( BYTE )p_byAlpha;
			m_tagInfo.m_bLastCIsValid = false;
		}
	}
}
//-------------------------------------------------------------------------
void CFKImage::SwapIndex( BYTE p_byIndex1, BYTE p_byIndex2 )
{
	SRGBQuad* pPal = GetPalette();
	if( pPal == NULL )
		return;
	if( m_pDib == NULL )
		return;
	SRGBQuad tmpRGB = GetPaletteColor( p_byIndex1 );
	SetPaletteColor( p_byIndex1, GetPaletteColor(p_byIndex2 ) );
	SetPaletteColor( p_byIndex2, tmpRGB );

	BYTE byIndex = 0;
	for( long y = 0; y < m_tagHead.biHeight; ++y )
	{
		for( long x = 0; x < m_tagHead.biWidth; ++x )
		{
			byIndex = _BlindGetPixelIndex( x, y );
			if( byIndex == p_byIndex1 )
				_BlindSetPixelIndex( x, y, p_byIndex2 );
			if( byIndex == p_byIndex2 )
				_BlindSetPixelIndex( x, y, p_byIndex1 );
		}
	}
}
//-------------------------------------------------------------------------
BYTE* CFKImage::GetBits( DWORD p_dwRow )
{
	if( m_pDib == NULL )
	{
		return NULL;
	}

	if( p_dwRow == 0 )
	{
		return (( BYTE* )m_pDib + *(DWORD*)m_pDib + GetPaletteSize() );
	}

	if( p_dwRow < ( DWORD )(m_tagHead.biHeight) )
	{
		return (( BYTE* )m_pDib + *(DWORD*)m_pDib + GetPaletteSize() + ( m_tagInfo.m_dwEffWidth * p_dwRow ) );
	}
	else
	{
		return NULL;
	}
	return NULL;
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetHeight() const
{
	return m_tagHead.biHeight;
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetWidth() const
{
	return m_tagHead.biWidth;
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetNumColors() const
{
	return m_tagHead.biClrUsed;
}
//-------------------------------------------------------------------------
WORD CFKImage::GetBpp() const
{
	return m_tagHead.biBitCount;
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetType() const
{
	return m_tagInfo.m_dwType;
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetEffWidth() const
{
	return m_tagInfo.m_dwEffWidth;
}
//-------------------------------------------------------------------------
BYTE CFKImage::GetJpegQuality() const
{
	return ( BYTE )( m_tagInfo.m_fQuality + 0.5f );
}
//-------------------------------------------------------------------------
void CFKImage::SetJpegQuality( BYTE p_byQuality )
{
	m_tagInfo.m_fQuality = ( float )p_byQuality;
}
//-------------------------------------------------------------------------
BYTE CFKImage::GetJpegScale() const
{
	return m_tagInfo.m_byJpegScale;
}
//-------------------------------------------------------------------------
void CFKImage::SetJpegScale( BYTE p_byScale )
{
	m_tagInfo.m_byJpegScale = p_byScale;
}
//-------------------------------------------------------------------------
void CFKImage::SetZipQuality( BYTE p_byQuality )
{
	if( p_byQuality > 9 )
		p_byQuality = 9;
	if( p_byQuality < 1 )
		p_byQuality = 1;
	m_tagInfo.m_byZipScale	= p_byQuality;
}
//-------------------------------------------------------------------------
BYTE CFKImage::GetZipQuality() const
{
	return m_tagInfo.m_byZipScale;
}
//-------------------------------------------------------------------------
bool CFKImage::IsGrayScale()
{
	SRGBQuad* pPal = GetPalette();
	if( !( m_pDib && pPal && m_tagHead.biClrUsed ) )
		return false;
	for( DWORD i = 0; i < m_tagHead.biClrUsed; ++i )
	{
		if( pPal[i].rgbBlue != i || pPal[i].rgbGreen != i || pPal[i].rgbRed != i )
			return false;
	}
	return true;
}
//-------------------------------------------------------------------------
DWORD CFKImage::GetCodecOption( DWORD p_dwImageType )
{
	if( p_dwImageType == 0 )
	{
		p_dwImageType = GetType();
	}
	return m_tagInfo.m_dwCodeOpt[p_dwImageType];
}
//-------------------------------------------------------------------------
bool CFKImage::SetCodecOption( DWORD p_dwOpt, DWORD p_dwImageType )
{
	if( p_dwImageType == 0 )
	{
		p_dwImageType = GetType();
	}
	m_tagInfo.m_dwCodeOpt[p_dwImageType] = p_dwOpt;
	return true;
}
//-------------------------------------------------------------------------
const char* CFKImage::GetLastError()
{
	return m_tagInfo.m_szLastError;
}
//-------------------------------------------------------------------------
bool CFKImage::IsInside( long p_lX, long p_lY )
{
	return ( 0 <= p_lY && p_lY < m_tagHead.biHeight &&
		0 <= p_lX && p_lX < m_tagHead.biWidth );
}
//-------------------------------------------------------------------------
bool CFKImage::Flip( bool p_bFlipSelection, bool p_bFlipAlpha )
{
	if( m_pDib == NULL )
		return false;

	BYTE* byBuffer = ( BYTE* )malloc( m_tagInfo.m_dwEffWidth );
	if( byBuffer == NULL )
		return false;

	BYTE* bySrc = NULL;
	BYTE* byDst	= NULL;
	bySrc = GetBits( m_tagHead.biHeight - 1 );
	byDst = GetBits( 0 );

	for( long i = 0; i < (m_tagHead.biHeight / 2); ++i )
	{
		memcpy( byBuffer,	bySrc,		m_tagInfo.m_dwEffWidth );
		memcpy( bySrc,		byDst,		m_tagInfo.m_dwEffWidth );
		memcpy( byDst,		byBuffer,	m_tagInfo.m_dwEffWidth );
		bySrc -= m_tagInfo.m_dwEffWidth;
		byDst += m_tagInfo.m_dwEffWidth;
	}
	free( byBuffer );

	if( p_bFlipSelection )
	{
#if USE_FK_SELECTION
		FlipSelection();
#endif
	}

	if( p_bFlipAlpha )
	{
#if USE_FK_ALPHA
		FilpAlpha();
#endif
	}

	return true;
}
//-------------------------------------------------------------------------
SRGBQuad CFKImage::GetPixelColor( long p_lX, long p_lY, bool p_bGetAlpha )
{
	SRGBQuad rgb = m_tagInfo.m_tagBkgndColor;
	if(( m_pDib == NULL ) || ( p_lX < 0 ) || ( p_lY < 0 ) ||
		( p_lX >= m_tagHead.biWidth ) || ( p_lY >= m_tagHead.biHeight ))
	{
		if( m_tagInfo.m_lBkgndIndex >= 0 )
		{
			if( m_tagHead.biBitCount < 24 )
				return GetPaletteColor((BYTE)m_tagInfo.m_lBkgndIndex );
			else
			{
				return m_tagInfo.m_tagBkgndColor;
			}
		}
		else if( m_pDib )
		{
			return GetPixelColor( 0, 0 );
		}
		return rgb;
	}

	if( m_tagHead.biClrUsed )
	{
		rgb = GetPaletteColor( _BlindGetPixelIndex( p_lX, p_lY ) );
	}
	else
	{
		BYTE* pDst = m_tagInfo.m_byImage + p_lY * m_tagInfo.m_dwEffWidth + p_lX * 3;
		rgb.rgbBlue		= *pDst++;
		rgb.rgbGreen	= *pDst++;
		rgb.rgbRed		= *pDst;
	}

#if USE_FK_ALPHA
	if( m_pAlpha && p_bGetAlpha )
		rgb.rgbReserved = _BlindGetAlpha( p_lX, p_lY );
#else
	rgb.rgbReserved = 0;
#endif
	return rgb;
}
//-------------------------------------------------------------------------
#if USE_FK_SELECTION
//-------------------------------------------------------------------------
bool CFKImage::DeleteSelection()
{
	if( m_pSelection )
	{
		free( m_pSelection );
		m_pSelection = NULL;
	}

	m_tagInfo.m_tagSelectionBox.left		= m_tagHead.biWidth;
	m_tagInfo.m_tagSelectionBox.bottom		= m_tagHead.biHeight;
	m_tagInfo.m_tagSelectionBox.right		= 0;
	m_tagInfo.m_tagSelectionBox.top			= 0;
	return true;
}
//-------------------------------------------------------------------------
bool CFKImage::FlipSelection()
{
	if( m_pSelection == NULL )
		return false;

	BYTE* pBuffer = ( BYTE* )malloc( m_tagHead.biWidth );
	if( pBuffer == NULL )
		return false;

	BYTE* bySrc = NULL;
	BYTE* byDst	= NULL ;
	bySrc = m_pSelection + ( m_tagHead.biHeight - 1 ) * m_tagHead.biWidth;
	byDst = m_pSelection;
	for( long i = 0; i < ( m_tagHead.biHeight / 2 ); ++i )
	{
		memcpy( pBuffer, bySrc, m_tagHead.biWidth );
		memcpy( bySrc, byDst, m_tagHead.biWidth );
		memcpy( byDst, pBuffer, m_tagHead.biWidth );
		bySrc -= m_tagHead.biWidth;
		byDst += m_tagHead.biWidth;
	}
	free( pBuffer );

	long lTop = m_tagInfo.m_tagSelectionBox.top;
	m_tagInfo.m_tagSelectionBox.top		= m_tagHead.biHeight - m_tagInfo.m_tagSelectionBox.bottom;
	m_tagInfo.m_tagSelectionBox.bottom	= m_tagHead.biHeight - lTop;
	return true;
}
//-------------------------------------------------------------------------
#endif
//-------------------------------------------------------------------------
#if USE_FK_ALPHA
//-------------------------------------------------------------------------
void CFKImage::DeleteAlpha()
{
	if( m_pAlpha )
	{
		free( m_pAlpha );
		m_pAlpha = NULL;
	}
}
//-------------------------------------------------------------------------
bool CFKImage::CreateAlpha()
{
	if( m_pAlpha == NULL )
	{
		m_pAlpha = ( BYTE* )malloc( m_tagHead.biWidth * m_tagHead.biHeight );
		if( m_pAlpha )
			memset( m_pAlpha, 255, m_tagHead.biWidth * m_tagHead.biHeight );
	}
	return ( m_pAlpha != NULL );
}
//-------------------------------------------------------------------------
bool CFKImage::IsAlphaValid()
{
	return ( m_pAlpha != NULL );
}
//-------------------------------------------------------------------------
void CFKImage::SetAlpha( const long p_lX, const long p_lY, const BYTE p_byLevel )
{
	if( m_pAlpha && IsInside( p_lX, p_lY ) )
	{
		m_pAlpha[ p_lX + p_lY * m_tagHead.biWidth ] = p_byLevel;
	}
}
//-------------------------------------------------------------------------
BYTE CFKImage::GetAlpha( const long p_lX, const long p_lY )
{
	if( m_pAlpha && IsInside( p_lX, p_lY ) )
		return m_pAlpha[p_lX + p_lY * m_tagHead.biWidth];
	return 0;
}
//-------------------------------------------------------------------------
void CFKImage::InvertAlpha()
{
	if( m_pAlpha )
	{
		BYTE* bySrc = m_pAlpha;
		long n = m_tagHead.biHeight * m_tagHead.biWidth;
		for( long i = 0; i < n; ++i )
		{
			*bySrc = ( BYTE )~( *(bySrc) );
			bySrc++;
		}
	}
}
//-------------------------------------------------------------------------
bool CFKImage::FilpAlpha()
{
	if( m_pAlpha == NULL )
		return false;

	BYTE* byBuff = ( BYTE* )malloc( m_tagHead.biWidth );
	if( byBuff == NULL )
		return false;

	BYTE* bySrc = NULL;
	BYTE* byDst = NULL;
	bySrc = m_pAlpha + ( m_tagHead.biHeight - 1 ) * m_tagHead.biWidth;
	byDst = m_pAlpha;
	for( long i = 0; i < ( m_tagHead.biHeight / 2 ); ++i )
	{
		memcpy( byBuff, bySrc, m_tagHead.biWidth );
		memcpy( bySrc, byDst, m_tagHead.biWidth );
		memcpy( byDst, byBuff, m_tagHead.biWidth );
		bySrc -= m_tagHead.biWidth;
		byDst += m_tagHead.biWidth;
	}
	free( byBuff );
	return true;
}
//-------------------------------------------------------------------------
BYTE* CFKImage::GetAlphaPointer( const long p_lX, const long p_lY )
{
	if( m_pAlpha && IsInside( p_lX, p_lY ) )
		return ( m_pAlpha + p_lX + p_lY * m_tagHead.biWidth );
	return NULL;
}
//-------------------------------------------------------------------------
BYTE CFKImage::_BlindGetAlpha( const long p_lX, const long p_lY )
{
#ifdef _DEBUG
	if( !IsInside( p_lX, p_lY ) || m_pAlpha == NULL )
		return 0;
#endif
	return m_pAlpha[ p_lX + p_lY * m_tagHead.biWidth ];
}
//-------------------------------------------------------------------------
#endif
//-------------------------------------------------------------------------
void CFKImage::_StartUp( DWORD p_dwImageType )
{
	m_pDib = m_pSelection = m_pAlpha = NULL;
	m_ppLayers = m_ppFrames = NULL;
	memset( &m_tagHead, 0, sizeof(SBitmapInfoHeader) );
	memset( &m_tagInfo, 0, sizeof(SFKImageInfo) );
	m_tagInfo.m_dwType		= p_dwImageType;
	m_tagInfo.m_fQuality	= 90.0f;
	m_tagInfo.m_byAlphaMax	= 255;
	m_tagInfo.m_lBkgndIndex	= -1;
	m_tagInfo.m_bEnabled	= true;
	SetXDpi( static_cast<long>(DEFAULT_DPI) );
	SetYDpi( static_cast<long>(DEFAULT_DPI) );
	m_tagInfo.m_bLittleEndianHost = _IsLittleEndian();
}
//-------------------------------------------------------------------------
bool CFKImage::_IsLittleEndian()
{
	short sTest = 1;
	return (*((char *) &sTest) == 1);
}
//-------------------------------------------------------------------------
void CFKImage::_CopyInfo( const CFKImage& p_Src )
{
	if( NULL == m_pDib )
	{
		memcpy( &m_tagInfo, &p_Src.m_tagInfo, sizeof( SFKImageInfo) );
	}
}
//-------------------------------------------------------------------------
void CFKImage::_Ghost(const CFKImage* p_pSrc )
{
	if( p_pSrc )
	{
		memcpy( &m_tagHead, &p_pSrc->m_tagHead, sizeof( SBitmapInfoHeader ) );
		memcpy( &m_tagInfo, &p_pSrc->m_tagInfo, sizeof( SFKImageInfo ) );
		m_pDib			= p_pSrc->m_pDib;
		m_pSelection	= p_pSrc->m_pSelection;
		m_pAlpha		= p_pSrc->m_pAlpha;
		m_ppLayers		= p_pSrc->m_ppLayers;
		m_ppFrames		= p_pSrc->m_ppFrames;

		m_tagInfo.m_pGhost	= (CFKImage*)p_pSrc;
	}
}
//-------------------------------------------------------------------------
long CFKImage::_Ntohl( const long p_lDword )
{
	if ( m_tagInfo.m_bLittleEndianHost ) 
		return p_lDword;
	return  ((p_lDword & 0xff) << 24 ) | ((p_lDword & 0xff00) << 8 ) |
		((p_lDword >> 8) & 0xff00) | ((p_lDword >> 24) & 0xff);
}
//-------------------------------------------------------------------------
short CFKImage::_Ntohs( const short p_sWord )
{
	if( m_tagInfo.m_bLittleEndianHost )
		return p_sWord;
	return ( ( (p_sWord & 0xff) << 8 ) | ( ( p_sWord >> 8 ) & 0xff ) );
}
//-------------------------------------------------------------------------
void CFKImage::_RGBtoBGR( BYTE* p_pBuffer, int p_nLength )
{
	if( p_pBuffer && (m_tagHead.biClrUsed == 0 ))
	{
		BYTE byTmp = 0;
		p_nLength = min( p_nLength, (int)m_tagInfo.m_dwEffWidth );
		p_nLength = min( p_nLength, (int)(3 * m_tagHead.biWidth) );
		for( int i = 0; i < p_nLength; i+=3 )
		{
			byTmp = p_pBuffer[i];
			p_pBuffer[i] = p_pBuffer[i+2];
			p_pBuffer[i+2] = byTmp;
		}
	}
}
//-------------------------------------------------------------------------
void CFKImage::_Bitfield2RGB( BYTE* p_pSrc, DWORD p_dwRedMask, 
								  DWORD p_dwGreenMask, DWORD p_dwBlueMask, BYTE p_byBpp)
{
	switch ( p_byBpp )
	{
	case 16:
		{
			DWORD dwNS[3]		= { 0, 0, 0 };
			for( int i = 0; i < 16; ++i )
			{
				if( (p_dwRedMask >> i) & 0x01 )
					dwNS[0]++;
				if( (p_dwGreenMask >> i) & 0x01 )
					dwNS[1]++;
				if( (p_dwBlueMask >> i) & 0x01 )
					dwNS[2]++;
			}

			// 将16位图像进行dword对齐
			dwNS[1] += dwNS[0];
			dwNS[2] += dwNS[1];
			dwNS[0] = 8 - dwNS[0];
			dwNS[1] -= 8;
			dwNS[2] -= 8;

			long lEffWidth = (( m_tagHead.biWidth + 1 ) / 2) * 4;
			WORD w = 0;
			long y2 = 0;
			long y3 = 0;
			long x2 = 0;
			long x3 = 0;
			BYTE* p = m_tagInfo.m_byImage;
			// 反向处理缓冲区以避免重新分配内存
			for( long y = m_tagHead.biHeight - 1; y >= 0; --y )
			{
				y2 = lEffWidth * y;
				y3 = m_tagInfo.m_dwEffWidth * y;
				for( long x = m_tagHead.biWidth - 1; x >= 0; --x )
				{
					x2 = 2 * x + y2;
					x3 = 3 * x + y3;
					w = ( WORD )( p_pSrc[x2] + 256 * p_pSrc[1+x2] );
					p[x3]	= ( BYTE )( ( w & p_dwBlueMask )<< dwNS[0] );
					p[1+x3]	= ( BYTE )( ( w & p_dwGreenMask )<< dwNS[1] );
					p[2+x3]	= ( BYTE )( ( w & p_dwRedMask )<< dwNS[2] );
				}
			}
		}
		break;
	case 32:
		{
			DWORD dwNS[3] = { 0, 0, 0 };
			for( int i = 8; i < 32; i += 8 )
			{
				if( p_dwRedMask >> i )
					dwNS[0]++;
				if( p_dwGreenMask >> i )
					dwNS[1]++;
				if( p_dwBlueMask >> i )
					dwNS[2]++;
			}

			long lEffWidth = m_tagHead.biWidth * 4;
			long y4 = 0;
			long y3 = 0;
			long x4 = 0;
			long x3 = 0;
			BYTE* p = m_tagInfo.m_byImage;
			// 反向处理缓冲区以避免重新分配内存
			for( long y = m_tagHead.biHeight - 1; y >= 0; --y )
			{
				y4 = lEffWidth * y;
				y3 = m_tagInfo.m_dwEffWidth * y;
				for( long x = m_tagHead.biWidth - 1; x >= 0; --x )
				{
					x4 = 4 * x + y4;
					x3 = 3 * x + y3;
					p[x3]	= p_pSrc[ dwNS[2] + x4 ];
					p[1+x3]	= p_pSrc[ dwNS[1] + x4 ];
					p[2+x3]	= p_pSrc[ dwNS[0] + x4 ];
				}
			}
		}
		break;
	default:
		break;
	}
}
//-------------------------------------------------------------------------
void CFKImage::_Bihtoh( SBitmapInfoHeader* p_pBih )
{
	p_pBih->biSize				=	_Ntohl( p_pBih->biSize );
	p_pBih->biWidth				=	_Ntohl( p_pBih->biWidth );
	p_pBih->biHeight			=	_Ntohl( p_pBih->biHeight );
	p_pBih->biPlanes			=	_Ntohs( p_pBih->biPlanes );
	p_pBih->biBitCount			=	_Ntohs( p_pBih->biBitCount );
	p_pBih->biCompression		=	_Ntohl( p_pBih->biCompression );
	p_pBih->biSizeImage			=	_Ntohl( p_pBih->biSizeImage );
	p_pBih->biXPelsPerMeter		=	_Ntohl( p_pBih->biXPelsPerMeter );
	p_pBih->biYPelsPerMeter		=	_Ntohl( p_pBih->biYPelsPerMeter );
	p_pBih->biClrUsed			=	_Ntohl( p_pBih->biClrUsed );
	p_pBih->biClrImportant		=	_Ntohl( p_pBih->biClrImportant );
}
//-------------------------------------------------------------------------
bool CFKImage::_EncodeSafeCheck( IFKFile* p_hFile )
{
	if( p_hFile == NULL )
	{
		strcpy( m_tagInfo.m_szLastError, ERR_NO_FILE );
		return true;
	}
	if( m_pDib == NULL )
	{
		strcpy( m_tagInfo.m_szLastError, ERR_NO_IMAGE );
		return true;
	}
	return false;
}
//-------------------------------------------------------------------------
BYTE CFKImage::_BlindGetPixelIndex( const long x, const long y )
{
#ifdef _DEBUG
	if( m_pDib == NULL )
		return 0;
	if( m_tagHead.biClrUsed == 0 )
		return 0;
	if( !IsInside( x, y ) )
		return 0;
#endif

	if( m_tagHead.biBitCount == 8 )
	{
		return m_tagInfo.m_byImage[ y * m_tagInfo.m_dwEffWidth + x ];
	}
	else
	{
		BYTE byPos = 0;
		BYTE byDst = m_tagInfo.m_byImage[ y * m_tagInfo.m_dwEffWidth + ( x * m_tagHead.biBitCount >> 3 )];
		if( m_tagHead.biBitCount == 4 )
		{
			byPos = ( BYTE )( 4 * ( 1 - x % 2 ));
			byDst &= ( 0x0F << byPos );
			return ( BYTE )( byDst >> byPos );
		}
		else if( m_tagHead.biBitCount == 1 )
		{
			byPos = ( BYTE )( 7 - x % 8 );
			byDst &= ( 0x01 << byPos );
			return ( BYTE )( byDst >> byPos );
		}
	}
	return 0;
}
//-------------------------------------------------------------------------
void CFKImage::_BlindSetPixelIndex( long x, long y, BYTE p_byByte )
{
#ifdef _DEBUG
	if( m_pDib == NULL )
		return;
	if( m_tagHead.biClrUsed == 0 )
		return;
	if( x < 0 || y < 0 || x >= m_tagHead.biWidth || y >= m_tagHead.biHeight )
		return;
#endif

	if( m_tagHead.biBitCount == 8 )
	{
		m_tagInfo.m_byImage[ y * m_tagInfo.m_dwEffWidth + x ] = p_byByte;
		return;
	}
	else
	{
		BYTE byPos = 0;
		BYTE* pDst = m_tagInfo.m_byImage + y * m_tagInfo.m_dwEffWidth + ( x * m_tagHead.biBitCount >> 3 );
		if( m_tagHead.biBitCount == 4 )
		{
			byPos = ( BYTE )( 4 * ( 1- x % 2 ));
			*pDst &= ~( 0x0f << byPos );
			*pDst |= ( (p_byByte & 0x0f) << byPos );
			return;
		}
		else if( m_tagHead.biBitCount == 1 )
		{
			byPos = ( BYTE )( 7 - x % 8 );
			*pDst &= ~( 0x01 << byPos );
			*pDst |= (( p_byByte & 0x01 ) << byPos );
			return;
		}
	}
}
//-------------------------------------------------------------------------
SRGBQuad CFKImage::_BlindGetPixelColor( long p_lX, long p_lY, bool p_bGetAlpha )
{
	SRGBQuad rgb;
	rgb.rgbReserved = 0;
#ifdef _DEBUG
	if( m_pDib == NULL )
		return rgb;
	if( !IsInside(p_lX, p_lY) )
		return rgb;
#endif

	if( m_tagHead.biClrUsed )
	{
		rgb = GetPaletteColor( _BlindGetPixelIndex( p_lX, p_lY ) );
	}
	else
	{
		BYTE* pDst = m_tagInfo.m_byImage + p_lY * m_tagInfo.m_dwEffWidth + p_lX * 3 ;
		rgb.rgbBlue		= *pDst++;
		rgb.rgbGreen	= *pDst++;
		rgb.rgbRed		= *pDst;
		rgb.rgbReserved	= 0;
	}

#if USE_FK_ALPHA
	if( m_pAlpha && p_bGetAlpha )
		rgb.rgbReserved = _BlindGetAlpha( p_lX, p_lY );
#endif
	return rgb;
}
//-------------------------------------------------------------------------
bool CFKImage::_IsHadFreeKnightSign( IFKFile* p_hFile )
{
	if( p_hFile == NULL )
		return false;
	if( !p_hFile->Seek( -10, SEEK_END ) )
		return false;
	char szSign[11];
	memset( szSign, 0, 11 );
	long lPos = p_hFile->Tell();
	if( p_hFile->Read( szSign, 10, 1 ) == 0 )
		return false;
	if( strcmp( "FreeKnight", szSign ) == 0 )
		return true;
	return false;
}
//-------------------------------------------------------------------------
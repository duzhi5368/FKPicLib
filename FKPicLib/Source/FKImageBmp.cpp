//-------------------------------------------------------------------------
#include "../Include/FKImageBmp.h"
#include "../Include/FKIOFile.h"
#include "../Include/FKImageIterator.h"
//-------------------------------------------------------------------------
CFKImageBMP::CFKImageBMP()
	: CFKImage( eFormats_BMP )
{

}
//-------------------------------------------------------------------------
bool CFKImageBMP::Decode( IFKFile* p_hFile )
{
	if( p_hFile == NULL )
		return false;

	SBitmapFileHeader tagFHeader;
	DWORD dwOff = p_hFile->Tell();

	if( p_hFile->Read( &tagFHeader, min(14, sizeof(tagFHeader)), 1 ) == 0 )
		return false;		// 不是bmp
	tagFHeader.bfSize		= _Ntohl( tagFHeader.bfSize );
	tagFHeader.bfOffBits	= _Ntohl( tagFHeader.bfOffBits );

	if( tagFHeader.bfType != BFT_BITMAP )
	{
		tagFHeader.bfOffBits	= 0;
		p_hFile->Seek( dwOff, SEEK_SET );
	}

	SBitmapInfoHeader tagIHeader;
	if( !_DibReadBitmapInfo( p_hFile, &tagIHeader ) )
		return false;		// Bmp图有问题

	DWORD dwCompression		= tagIHeader.biCompression;
	DWORD dwBitCount		= tagIHeader.biBitCount;
	bool bIsOldBmp			= (tagIHeader.biSize == sizeof( SBitmapCoreHeader ));
	bool bIsTopDownDib		= (tagIHeader.biHeight < 0);
	if( bIsTopDownDib )
	{
		tagIHeader.biHeight = -tagIHeader.biHeight;
	}

	if( m_tagInfo.m_lEscape == -1 )
	{
		m_tagHead.biWidth	= tagIHeader.biWidth;
		m_tagHead.biHeight	= tagIHeader.biHeight;
		m_tagInfo.m_dwType	= eFormats_BMP;
	}

	// 创建真实对象
	if( !Create( tagIHeader.biWidth, tagIHeader.biHeight, tagIHeader.biBitCount, eFormats_BMP ))
		return false;

	SetXDpi( static_cast<long>( tagIHeader.biXPelsPerMeter * 254.0 / 10000.0 + 0.5 ) );
	SetYDpi( static_cast<long>( tagIHeader.biYPelsPerMeter * 254.0 / 10000.0 + 0.5 ) );

	if( m_tagInfo.m_lEscape )
		return false;

	SRGBQuad* pRGB = GetPalette();
	if( pRGB )
	{
		if( bIsOldBmp )
		{
			// 将3字节的老格式转换为新的4字节新格式
			p_hFile->Read((void*)pRGB, DibNumColors(&tagIHeader) * sizeof(SRgbTriple), 1 );
			for( int i = DibNumColors(&m_tagHead) - 1; i >= 0; --i )
			{
				pRGB[i].rgbRed			= ((SRgbTriple*)pRGB)[i].rgbtRed;
				pRGB[i].rgbGreen		= ((SRgbTriple*)pRGB)[i].rgbtGreen;
				pRGB[i].rgbBlue			= ((SRgbTriple*)pRGB)[i].rgbtBlue;
				pRGB[i].rgbReserved		= (BYTE)0;
			}
		}
		else
		{
			p_hFile->Read((void*)pRGB, DibNumColors(&tagIHeader) * sizeof(SRGBQuad), 1 );
			// 强制清除保留位信息，解决一些winXp的BMP bug
			for( unsigned int i = 0; i < m_tagHead.biClrUsed; ++i )
				pRGB[i].rgbReserved = 0;
		}
	}

	if( m_tagInfo.m_lEscape )
		return false;

	switch( dwBitCount )
	{
	case 32:
		{
			DWORD dwMask[3];
			if( dwCompression == eBI_BITFIELDS )
			{
				p_hFile->Read( dwMask, 12, 1 );
			}
			else
			{
				dwMask[0] = 0x00FF0000;
				dwMask[0] = 0x0000FF00;
				dwMask[0] = 0x000000FF;
			}

			if( tagFHeader.bfOffBits != 0 )
			{
				p_hFile->Seek( dwOff + tagFHeader.bfOffBits, SEEK_SET );
			}

			if( dwCompression == eBI_BITFIELDS || dwCompression == eBI_RGB )
			{
				long lImageSize = 4 * m_tagHead.biHeight * m_tagHead.biWidth;
				BYTE* pBuffer32 = ( BYTE* )malloc( lImageSize );
				if( pBuffer32 )
				{
					p_hFile->Read( pBuffer32, lImageSize, 1 );

#if USE_FK_ALPHA
					if( dwCompression == eBI_RGB )
					{
						CreateAlpha();
						if( IsAlphaValid() )
						{
							bool bIsAlphaOK = false;
							BYTE* p = NULL;
							for( long y = 0; y < m_tagHead.biHeight; ++y )
							{
								p = pBuffer32 + 3 + m_tagHead.biWidth * 4 * y;
								for( long x = 0; x < m_tagHead.biWidth; ++x )
								{
									if( *p )
									{
										bIsAlphaOK = true;
									}
									SetAlpha( x, y, *p );
									p += 4;
								}
							}

							if( !bIsAlphaOK )
							{
								InvertAlpha();
							}
						}
					}
#endif
					_Bitfield2RGB( pBuffer32, dwMask[0], dwMask[1], dwMask[2], 32 );
					free( pBuffer32 );
				}
			}
		}
		break;
	case 24:
		{
			if( tagFHeader.bfOffBits != 0 )
			{
				p_hFile->Seek( dwOff + tagFHeader.bfOffBits, SEEK_SET );
			}
			if( dwCompression == eBI_RGB )
			{
				p_hFile->Read( m_tagInfo.m_byImage, m_tagHead.biSizeImage, 1 );	// 逐字节读取
			}
		}
		break;
	case 16:
		{
			DWORD dwMask[3] = { 0, 0, 0 };
			if( dwCompression == eBI_BITFIELDS )
			{
				p_hFile->Read( dwMask, 12, 1 );
			}
			else
			{
				dwMask[0] = 0x7C00;
				dwMask[1] = 0x03E0;
				dwMask[2] = 0x001F;		// rgb555
			}

			if( tagFHeader.bfOffBits != 0 )
				p_hFile->Seek( dwOff + tagFHeader.bfOffBits, SEEK_SET );
			p_hFile->Read( m_tagInfo.m_byImage, m_tagHead.biHeight * ((m_tagHead.biWidth + 1)/2) * 4, 1 );
			_Bitfield2RGB( m_tagInfo.m_byImage, dwMask[0], dwMask[1], dwMask[2], 16 );
		}
		break;
	case 8:
	case 4:
	case 1:
		{
			if( tagFHeader.bfOffBits != 0 )
			{
				p_hFile->Seek( dwOff + tagFHeader.bfOffBits , SEEK_SET );
			}
			switch ( dwCompression )
			{
			case eBI_RGB:
				{
					p_hFile->Read( m_tagInfo.m_byImage, m_tagHead.biSizeImage, 1 );
				}
				break;
			case eBI_RLE4:
				{
					BYTE byStatus = 0;
					BYTE bySecond = 0;
					int nScanLine = 0;
					int nBits = 0;
					bool bLowNibble = false;
					
					CFKImageIterator Iter(this);
					for( bool bContinue = true; bContinue && p_hFile->Read(&byStatus, sizeof(BYTE), 1); )
					{
						switch ( byStatus )
						{
						case RLE_COMMAND:
							{
								p_hFile->Read( &byStatus, sizeof(BYTE), 1 );
								switch( byStatus )
								{
								case RLE_ENDOFLINE:
									nBits = 0;
									nScanLine++;
									bLowNibble = false;
									break;
								case RLE_ENDOFBITMAP:
									bContinue = false;
									break;
								case RLE_DELTA:
									BYTE byDeltaX;
									BYTE byDeltaY;
									p_hFile->Read( &byDeltaX, sizeof(BYTE), 1 );
									p_hFile->Read( &byDeltaY, sizeof(BYTE), 1 );
									nBits		+= (byDeltaX / 2);
									nScanLine	+= byDeltaY;
									break;
								default:
									p_hFile->Read( &bySecond, sizeof(BYTE), 1 );
									BYTE* pLine = Iter.GetRow( nScanLine );
									for( int i = 0; i < byStatus; ++i )
									{
										if(( BYTE* )( pLine + nBits ) < ( BYTE* )( m_tagInfo.m_byImage + m_tagHead.biSizeImage ))
										{
											if( bLowNibble )
											{
												if( i & 1 )
												{
													*(pLine + nBits) |= ( bySecond & 0x0f );
												}
												else
												{
													*(pLine + nBits) |= ( bySecond & 0xf0 ) >> 4;
												}

												nBits++;
											}
											else
											{
												if( i & 1 )
												{
													*(pLine + nBits) = (BYTE)( bySecond & 0x0f ) << 4;
												}
												else
												{
													*(pLine + nBits) = (BYTE)( bySecond & 0xf0 );
												}
											}
										}

										if( ( i & 1 ) && ( i != (byStatus - 1 )) )
										{
											p_hFile->Read( &bySecond, sizeof(BYTE), 1 );
										}

										bLowNibble = !bLowNibble;
									}
									if(((( byStatus + 1 ) >> 1 ) & 1 ) == 1 )
									{
										p_hFile->Read( &bySecond, sizeof(BYTE), 1 );
									}
									break;
								}
							}
							break;
						default:
							{
								BYTE* pLine = Iter.GetRow( nScanLine );
								p_hFile->Read( &bySecond, sizeof(BYTE), 1 );
								for( unsigned int i = 0; i < byStatus; ++i )
								{
									if(( BYTE* )( pLine + nBits )< (BYTE*)( m_tagInfo.m_byImage + m_tagHead.biSizeImage ))
									{
										if( bLowNibble )
										{
											if( i & 1 )
											{
												*(pLine + nBits) |= ( bySecond & 0x0f );
											}
											else
											{
												*(pLine + nBits) |= ( bySecond & 0xf0 ) >> 4;
											}

											nBits++;
										}
										else
										{
											if( i & 1 )
											{
												*(pLine + nBits) = (BYTE)( bySecond & 0x0f ) << 4;
											}
											else
											{
												*(pLine + nBits) = (BYTE)( bySecond & 0xf0 );
											}
										}
									}
									bLowNibble = !bLowNibble;
								}
							}
							break;
						}
					}
				}
				break;
			case eBI_RLE8:
				{
					BYTE byStatus	= 0;
					BYTE bySecond	= 0;
					int nScanLine = 0;
					int nBits = 0;

					CFKImageIterator Iter(this);
					for( bool bContinue = true; bContinue && p_hFile->Read(&byStatus, sizeof(BYTE), 1 ); )
					{
						switch ( byStatus )
						{
						case RLE_ENDOFLINE:
							nBits = 0;
							nScanLine++;
							break;
						case RLE_ENDOFBITMAP:
							bContinue = false;
							break;
						case RLE_DELTA:
							BYTE byDeltaX;
							BYTE byDeltaY;
							p_hFile->Read( &byDeltaX, sizeof(BYTE), 1 );
							p_hFile->Read( &byDeltaY, sizeof(BYTE), 1 );
							nBits		+= byDeltaX;
							nScanLine	+= byDeltaY;
							break;
						default:
							p_hFile->Read((void*)(Iter.GetRow(nScanLine) + nBits), sizeof(BYTE) * byStatus, 1 );
							if( (byStatus & 1) == 1 )
							{
								p_hFile->Read(&bySecond, sizeof(BYTE), 1);
							}
							nBits += byStatus;
							break;
						}
					}
				}
				break;
			default:
				break;
			}
		}
		break;
	}

	if( bIsTopDownDib )
		Flip();

	return true;
}
//-------------------------------------------------------------------------
bool CFKImageBMP::Decode( FILE* p_hFile )
{
	CFKIOFile file( p_hFile );
	return Decode( &file );
}
//-------------------------------------------------------------------------
bool CFKImageBMP::Encode( IFKFile* p_hFile )
{
	if( _EncodeSafeCheck(p_hFile) )
		return false;

	SBitmapFileHeader FHeader;
	FHeader.bfType		= BFT_BITMAP;
	FHeader.bfSize		= GetSize() + 14;
	FHeader.bfReserved1	= FHeader.bfReserved2	= 0;
	FHeader.bfOffBits	= 14 + m_tagHead.biSize + GetPaletteSize();
	FHeader.bfType		= _Ntohs( FHeader.bfType );
	FHeader.bfSize		= _Ntohl( FHeader.bfSize );
	FHeader.bfOffBits	= _Ntohl( FHeader.bfOffBits );

#if USE_FK_ALPHA
	if( GetNumColors() == 0 && IsAlphaValid() )
	{
		SBitmapInfoHeader IHeader;
		memcpy( &IHeader, &m_tagHead, sizeof(SBitmapInfoHeader) );
		IHeader.biCompression	= eBI_RGB;
		IHeader.biBitCount		= 32;
		DWORD dwEffWidth		= (((IHeader.biBitCount * IHeader.biWidth) + 31) / 32 ) * 4;
		IHeader.biSizeImage		= dwEffWidth * IHeader.biHeight;
		
		FHeader.bfSize			= IHeader.biSize + IHeader.biSizeImage + 14;
		FHeader.bfSize			= _Ntohl( FHeader.bfSize );
		_Bihtoh( &IHeader );

		p_hFile->Write( &FHeader, min(14, sizeof(SBitmapFileHeader)), 1 );
		p_hFile->Write( &IHeader, sizeof( SBitmapInfoHeader ), 1 );
		BYTE* pSrcAlpha = GetAlphaPointer();
		for( long y = 0; y < IHeader.biHeight; ++y )
		{
			BYTE* pSrc = GetBits( y );
			for( long x = 0; x < IHeader.biWidth; ++x )
			{
				p_hFile->Write( pSrc, 3, 1 );
				p_hFile->Write( pSrcAlpha, 1, 1 );
				pSrc += 3;
				++pSrcAlpha;
			}
		}
	}
	else
#endif
	{
		p_hFile->Write( &FHeader, min(14, sizeof(SBitmapFileHeader)), 1 );
		memcpy( m_pDib, &m_tagHead, sizeof( SBitmapInfoHeader ) );
		_Bihtoh( (SBitmapInfoHeader*)m_pDib );
		p_hFile->Write( m_pDib, GetSize(), 1 );
		_Bihtoh( (SBitmapInfoHeader*)m_pDib );
	}
	return true;
}
//-------------------------------------------------------------------------
bool CFKImageBMP::Encode( FILE* p_hFile )
{
	CFKIOFile file( p_hFile );
	return Encode( &file );
}
//-------------------------------------------------------------------------
bool CFKImageBMP::_DibReadBitmapInfo( IFKFile* p_hFile, SBitmapInfoHeader* p_pDib )
{
	if( p_hFile == NULL || p_pDib == NULL )
		return false;

	if( p_hFile->Read( p_pDib, sizeof(SBitmapInfoHeader), 1 ) == 0 )
		return false;

	_Bihtoh(p_pDib);

	switch ( p_pDib->biSize )
	{
	case sizeof(SBitmapInfoHeader):
		break;
	case 64: // sizeof( OS2_BMP_HEADER )
		p_hFile->Seek((long)( 64 - sizeof(SBitmapInfoHeader)), SEEK_CUR );
		break;
	case sizeof(SBitmapCoreHeader):
		{
			SBitmapCoreHeader CHeader	= *(SBitmapCoreHeader*)p_pDib;
			p_pDib->biSize				= CHeader.bcSize;
			p_pDib->biWidth				= (DWORD)CHeader.bcWidth;
			p_pDib->biHeight			= (DWORD)CHeader.bcHeight;
			p_pDib->biPlanes			= CHeader.bcPlanes;
			p_pDib->biBitCount			= CHeader.bcBitCount;
			p_pDib->biCompression		= eBI_RGB;
			p_pDib->biSizeImage			= 0;
			p_pDib->biXPelsPerMeter		= p_pDib->biYPelsPerMeter	= 0;
			p_pDib->biClrImportant		= p_pDib->biClrUsed			= 0;

			p_hFile->Seek( (long)(sizeof(SBitmapCoreHeader) - sizeof(SBitmapInfoHeader)), SEEK_CUR );
		}
		break;
	default:
		if( p_pDib->biSize > sizeof( SBitmapInfoHeader ) &&
			( p_pDib->biSizeImage >= ( DWORD )( p_pDib->biHeight * (((( p_pDib->biBitCount * p_pDib->biWidth ) + 31 ) / 32 ) * 4 ) ) ) &&
			( p_pDib->biPlanes == 1 ) &&
			( p_pDib->biClrUsed == 0 ))
		{
			if( p_pDib->biCompression == eBI_RGB )
				p_hFile->Seek( (long)(p_pDib->biSize - sizeof(SBitmapInfoHeader)), SEEK_CUR );
		}
		break;
	}

	FixBitmapInfo( p_pDib );

	return true;
}
//-------------------------------------------------------------------------
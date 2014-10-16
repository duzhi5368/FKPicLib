//-------------------------------------------------------------------------
#include "../Include/FKImagePng.h"
#include "../Include/FKImageIterator.h"
#include "../Include/FKIOFile.h"
//-------------------------------------------------------------------------
CFKImagePNG::CFKImagePNG()
	: CFKImage( eFormats_PNG )
{

}
//-------------------------------------------------------------------------
CFKImagePNG::~CFKImagePNG()
{

}
//-------------------------------------------------------------------------
bool CFKImagePNG::Decode( IFKFile* p_hFile )
{
	png_struct*			pPngPtr = NULL;
	png_info*			pInfoPtr = NULL;
	BYTE*				pRowPointer = NULL;
	CFKImageIterator	Iter(this );

	pPngPtr = png_create_read_struct( PNG_LIBPNG_VER_STRING, (void*)NULL, NULL, NULL );
	if( NULL == pPngPtr )
	{
		strcpy( m_tagInfo.m_szLastError, "Failed to create PNG structure" );
		return false;
	}
	pInfoPtr = png_create_info_struct( pPngPtr );
	if( NULL == pInfoPtr )
	{
		strcpy( m_tagInfo.m_szLastError, "Failed to initialize PNG info structure" );
		return false;
	}
	if( setjmp( pPngPtr->jmpbuf ) )
	{
		delete [] pRowPointer;
		png_destroy_read_struct( &pPngPtr, &pInfoPtr, (png_infopp)NULL );
		return false;
	}

	png_set_read_fn( pPngPtr, p_hFile, MyReadDataFunc );
	png_set_error_fn( pPngPtr, m_tagInfo.m_szLastError, MyErrorFunc, NULL );
	png_read_info( pPngPtr, pInfoPtr );

	if( m_tagInfo.m_lEscape == -1 )
	{
		m_tagHead.biWidth	= pInfoPtr->width;
		m_tagHead.biHeight	= pInfoPtr->height;
		m_tagInfo.m_dwType	= eFormats_PNG;
		longjmp( pPngPtr->jmpbuf, 1 );
	}
	
	int nChannel = 0;
	switch ( pInfoPtr->color_type )
	{
	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_PALETTE:
		nChannel = 1;
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		nChannel = 2;
		break;
	case PNG_COLOR_TYPE_RGB:
		nChannel = 3;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		nChannel = 4;
		break;
	default:
		strcpy( m_tagInfo.m_szLastError,"unknown PNG color type");
		longjmp( pPngPtr->jmpbuf, 1);
		break;
	}

	int nPixelDepth = pInfoPtr->pixel_depth;
	if( nChannel == 1 && nPixelDepth > 8 )
		nPixelDepth = 8;
	if( nChannel == 2 )
		nPixelDepth = 8;
	if( nChannel == 3 )
		nPixelDepth = 24;

	if( !Create( pPngPtr->width, pPngPtr->height, nPixelDepth, eFormats_PNG ) )
	{
		longjmp( pPngPtr->jmpbuf, 1 );
	}

	switch ( pInfoPtr->phys_unit_type )
	{
	case PNG_RESOLUTION_UNKNOWN:
		SetXDpi( pInfoPtr->x_pixels_per_unit );
		SetYDpi( pInfoPtr->y_pixels_per_unit );
		break;
	case PNG_RESOLUTION_METER:
		SetXDpi( (long)floor( pInfoPtr->x_pixels_per_unit * 254.0 / 10000.0 + 0.5 ));
		SetYDpi( (long)floor( pInfoPtr->y_pixels_per_unit * 254.0 / 10000.0 + 0.5 ));
		break;
	default:
		break;
	}

	if( pInfoPtr->num_palette > 0 )
	{
		SetPalette( (SRGBColor*)pInfoPtr->palette, pInfoPtr->num_palette );
		SetClrImportant( pInfoPtr->num_palette );
	}
	else if( pPngPtr->bit_depth == 2 )
	{
		SetPaletteColor( 0, 0, 0, 0 );
		SetPaletteColor( 1, 85, 85, 85 );
		SetPaletteColor( 2, 170, 170, 170 );
		SetPaletteColor( 3, 255, 255, 255 );
	}
	else
	{
		SetGrayPalette();
	}

	int nShift = max( 0, (pInfoPtr->bit_depth >> 3) - 1 ) << 3;
	if( pInfoPtr->num_trans != 0 )
	{
		if( pInfoPtr->num_trans == 1 )
		{
			if( pInfoPtr->color_type == PNG_COLOR_TYPE_PALETTE )
			{
				m_tagInfo.m_lBkgndIndex = pInfoPtr->trans_values.index;
			}
			else
			{
				m_tagInfo.m_lBkgndIndex	 = pInfoPtr->trans_values.gray >> nShift;
			}
		}
		else if( pInfoPtr->num_trans > 1 )
		{
			SRGBQuad* pPal = GetPalette();
			if( pPal )
			{
				DWORD dwN = 0;
				for( dwN = 0; dwN < min( m_tagHead.biClrUsed, (unsigned long)pInfoPtr->num_trans ); ++dwN )
				{
					pPal[dwN].rgbReserved = pInfoPtr->trans[dwN];
				}
				for( dwN = pInfoPtr->num_trans; dwN < m_tagHead.biClrUsed; ++dwN )
				{
					pPal[dwN].rgbReserved = 255;
				}
				m_tagInfo.m_bAlphaPaletteEnable = true;
			}
		}
	}

	if( nChannel == 3 )
	{
		png_bytep pTrans = 0;
		int nNumTrans = 0;
		png_color_16* pImageBackground = 0;
		if( png_get_tRNS( pPngPtr, pInfoPtr, &pTrans, &nNumTrans, &pImageBackground ) )
		{
			m_tagInfo.m_tagBkgndColor.rgbRed		=	(BYTE)(( pInfoPtr->trans_values.red )	>> nShift );
			m_tagInfo.m_tagBkgndColor.rgbGreen		=	(BYTE)(( pInfoPtr->trans_values.green ) >> nShift );
			m_tagInfo.m_tagBkgndColor.rgbBlue		=	(BYTE)(( pInfoPtr->trans_values.blue )	>> nShift );
			m_tagInfo.m_tagBkgndColor.rgbReserved	=	0;
			m_tagInfo.m_lBkgndIndex					=	0;
		}
	}

	int nAlphaPresent = ( nChannel - 1 ) % 2;
	if( nAlphaPresent )
	{
#if USE_FK_ALPHA
		CreateAlpha();
#else
		png_set_strip_alpha( pPngPtr );
#endif
	}

	if( pInfoPtr->color_type & PNG_COLOR_MASK_COLOR )
	{
		png_set_bgr( pPngPtr );
	}
	if( m_tagInfo.m_lEscape )
	{
		longjmp( pPngPtr->jmpbuf, 1 );
	}

	pRowPointer = new BYTE[ pInfoPtr->rowbytes + 8 ];
	int nNumberPasses = png_set_interlace_handling( pPngPtr );
	if( nNumberPasses > 1 )
	{
		SetCodecOption( 1 );
	}
	else
	{
		SetCodecOption( 0 );
	}

	int nChanOffset = pInfoPtr->bit_depth >> 3;
	int nPixelOffset = pInfoPtr->pixel_depth >> 3;
	for( int nPass = 0; nPass < nNumberPasses; ++nPass )
	{
		Iter.Upset();
		int y = 0;
		do 
		{
			if( m_tagInfo.m_lEscape )
				longjmp( pPngPtr->jmpbuf, 1 );
#if USE_FK_ALPHA
			if( IsAlphaValid() )
			{
				long lX = 0;
				long lY = 0;
				lY = m_tagHead.biHeight - 1 - y;
				BYTE* pRow = Iter.GetRow( lY );
				if( pInfoPtr->interlace_type && nPass > 0 && nPass != 7 )
				{
					for( lX = 0; lX < m_tagHead.biWidth; lX++ )
					{
						long lPx = lX * nPixelOffset;
						if( nChannel == 2 )
						{
							pRowPointer[lPx]					= pRow[lX];
							pRowPointer[lPx + nChanOffset ]		= GetAlpha( lX, lY );
						}
						else
						{
							long qx = lX * 3;
							pRowPointer[lPx]					= pRow[qx];
							pRowPointer[lPx + nChanOffset]		= pRow[qx+1];
							pRowPointer[lPx + nChanOffset * 2]	= pRow[qx+2];
							pRowPointer[lPx + nChanOffset * 3]	= GetAlpha( lX, lY );
						}
					}
				}

				png_read_row( pPngPtr, pRowPointer, NULL );

				// RGBA ×ª»»Îª RGB + A
				for( lX = 0; lX < m_tagHead.biWidth; lX++ )
				{
					long lPx = lX * nPixelOffset;
					if( nChannel == 2 )
					{
						pRow[lX]		= pRowPointer[lPx];
						SetAlpha( lX, lY, pRowPointer[lPx + nChanOffset] );
					}
					else
					{
						long qx = lX * 3;
						pRow[qx]		= pRowPointer[lPx];
						pRow[qx+1]		= pRowPointer[lPx + nChanOffset];
						pRow[qx+2]		= pRowPointer[lPx + nChanOffset * 2];
						SetAlpha( lX, lY, pRowPointer[lPx + nChanOffset * 3] );
					}
				}
			}
			else
#endif
			{
				if( pInfoPtr->interlace_type && nPass > 0 )
				{
					Iter.GetRow( pRowPointer, pInfoPtr->rowbytes );
					if( pInfoPtr->bit_depth > 8 )
					{
						for( long lx = ( m_tagHead.biWidth * nChannel -1 ); lx >= 0; --lx )
						{
							pRowPointer[lx * nChanOffset] = pRowPointer[lx];
						}
					}
				}

				png_read_row( pPngPtr, pRowPointer, NULL );

				if( pInfoPtr->bit_depth > 8 )
				{
					for( long lx = 0; lx < ( m_tagHead.biWidth * nChannel ); lx++ )
					{
						pRowPointer[lx] = pRowPointer[lx * nChanOffset];
					}
				}

				Iter.SetRow( pRowPointer, pInfoPtr->rowbytes );

				if( pInfoPtr->bit_depth == 2 && nPass == (nNumberPasses - 1) )
				{
					_Expand2To4Bpp( Iter.GetRow() );
				}

				Iter.PrewRow();
			}
			
			y++;
		} while ( y < m_tagHead.biHeight );
	}

	delete [] pRowPointer;

	png_read_end( pPngPtr, pInfoPtr );
	png_destroy_read_struct( &pPngPtr, &pInfoPtr, (png_infopp)NULL );
	return true;
}
//-------------------------------------------------------------------------
bool CFKImagePNG::Decode( FILE* p_hFile )
{
	CFKIOFile file( p_hFile );
	return Decode( &file );
}
//-------------------------------------------------------------------------
bool CFKImagePNG::Encode( IFKFile* p_hFile )
{
	if( _EncodeSafeCheck(p_hFile) )
		return false;

	CFKImageIterator Iter( this );
	BYTE byTrans[256];
	png_struct* pPngPtr = NULL;
	png_info*	pInfoPtr = NULL;

	pPngPtr = png_create_write_struct( PNG_LIBPNG_VER_STRING, (void*)NULL, NULL, NULL );
	if( pPngPtr == NULL )
		return false;
	pInfoPtr = png_create_info_struct( pPngPtr );
	if( pInfoPtr == NULL )
	{
		png_destroy_write_struct( &pPngPtr, (png_infopp)NULL );
		return false;
	}

	if( setjmp(pPngPtr->jmpbuf) )
	{
		if( pInfoPtr->palette )
			free( pInfoPtr->palette );
		png_destroy_write_struct( &pPngPtr, (png_infopp)&pInfoPtr );
		return false;
	}

	png_set_write_fn( pPngPtr, p_hFile, MyWriteDataFunc, MyFlushDataFunc );

	pInfoPtr->width = GetWidth();
	pInfoPtr->height = GetHeight();
	pInfoPtr->pixel_depth = (BYTE)GetBpp();
	pInfoPtr->channels = ( GetBpp() > 8 )?(BYTE)3 : (BYTE)1;
	pInfoPtr->bit_depth = (BYTE)( GetBpp() / pInfoPtr->channels );
	pInfoPtr->compression_type = pInfoPtr->filter_type = 0;
	pInfoPtr->valid = 0;

	switch ( GetCodecOption( eFormats_PNG ) )
	{
	case 1:
		pInfoPtr->interlace_type = PNG_INTERLACE_ADAM7;
		break;
	default:
		pInfoPtr->interlace_type = PNG_INTERLACE_NONE;
		break;
	}

	bool bIsGrayScale = IsGrayScale();
	if( GetNumColors() )
	{
		if( bIsGrayScale )
		{
			pInfoPtr->color_type = PNG_COLOR_TYPE_GRAY;
		}
		else
		{
			pInfoPtr->color_type = PNG_COLOR_TYPE_PALETTE;
		}
	}
	else
	{
		pInfoPtr->color_type = PNG_COLOR_TYPE_RGB;
	}

#if USE_FK_ALPHA
	if( IsAlphaValid() )
	{
		pInfoPtr->color_type		|= PNG_COLOR_MASK_ALPHA;
		pInfoPtr->channels++;
		pInfoPtr->bit_depth = 8;
		pInfoPtr->pixel_depth += 8;
	}
#endif

	png_color_16 ImageBackground = { 0, 255, 255, 255, 0 };
	SRGBQuad tagTc = GetTransColor();
	if( m_tagInfo.m_lBkgndIndex >= 0 )
	{
		ImageBackground.blue	= tagTc.rgbBlue;
		ImageBackground.green	= tagTc.rgbGreen;
		ImageBackground.red		= tagTc.rgbRed;
	}

	png_set_bKGD( pPngPtr, pInfoPtr, &ImageBackground );
	png_set_pHYs( pPngPtr, pInfoPtr, m_tagHead.biXPelsPerMeter, m_tagHead.biYPelsPerMeter, PNG_RESOLUTION_METER );
	png_set_IHDR( pPngPtr, pInfoPtr, pInfoPtr->width, pInfoPtr->height, pInfoPtr->bit_depth,
		pInfoPtr->color_type, pInfoPtr->interlace_type, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );

	if( m_tagInfo.m_lBkgndIndex > 0 )
	{
		pInfoPtr->num_trans					= 1;
		pInfoPtr->valid						|= PNG_INFO_tRNS;
		pInfoPtr->trans						= byTrans;
		pInfoPtr->trans_values.index		= (BYTE)m_tagInfo.m_lBkgndIndex;
		pInfoPtr->trans_values.red			= tagTc.rgbRed;
		pInfoPtr->trans_values.green		= tagTc.rgbGreen;
		pInfoPtr->trans_values.blue			= tagTc.rgbBlue;
		pInfoPtr->trans_values.gray			= pInfoPtr->trans_values.index;

		if( !bIsGrayScale && m_tagHead.biClrUsed && m_tagInfo.m_lBkgndIndex )
		{
			SwapIndex( 0, (BYTE)m_tagInfo.m_lBkgndIndex );
		}
	}


	if( GetPalette() )
	{
		if( !bIsGrayScale )
		{
			pInfoPtr->valid |= PNG_INFO_PLTE;
		}

		int nc = GetClrImportant();
		if( nc == 0 )
			nc = GetNumColors();

		if( m_tagInfo.m_bAlphaPaletteEnable )
		{
			for( WORD wI = 0; wI < nc; wI++ )
			{
				byTrans[wI] = GetPaletteColor((BYTE)wI).rgbReserved;
			}
			pInfoPtr->num_trans		= (WORD)nc;
			pInfoPtr->valid			|= PNG_INFO_tRNS;
			pInfoPtr->trans			= byTrans;
		}

		pInfoPtr->palette		= new png_color[nc];
		pInfoPtr->num_palette	= ( png_uint_16 )nc;
		for( int i = 0; i < nc; ++i )
		{
			GetPaletteColor( i, &pInfoPtr->palette[i].red, 
				&pInfoPtr->palette[i].green, &pInfoPtr->palette[i].blue );
		}
	}

#if USE_FK_ALPHA
	if( IsAlphaValid() && m_tagHead.biBitCount == 24 && m_tagInfo.m_lBkgndIndex >= 0 )
	{
		for( long y = 0; y < m_tagHead.biHeight; ++y )
		{
			for( long x = 0; x < m_tagHead.biWidth; ++x )
			{
				SRGBQuad c = GetPixelColor( x, y, false );
				if( *(long*)&c == *(long*)&tagTc )
				{
					SetAlpha( x, y ,0 );
				}
			}
		}
	}
#endif

	int nRowSize = max( m_tagInfo.m_dwEffWidth, pInfoPtr->width * pInfoPtr->channels * ( pInfoPtr->bit_depth / 8 ) );
	pInfoPtr->rowbytes = nRowSize;
	BYTE* pRowPointer = new BYTE[nRowSize];

	png_write_info( pPngPtr, pInfoPtr );

	int nNumPass = png_set_interlace_handling( pPngPtr );
	for( int nPass = 0; nPass < nNumPass; ++nPass )
	{
		Iter.Upset();
		long ay = m_tagHead.biHeight - 1;
		SRGBQuad tagRGB;
		do{
#if USE_FK_ALPHA
			if( IsAlphaValid() )
			{
				for( long ax = m_tagHead.biWidth - 1; ax >= 0; ax-- )
				{
					tagRGB = _BlindGetPixelColor( ax, ay );
					int px = ax * pInfoPtr->channels;
					if( !bIsGrayScale )
					{
						pRowPointer[px++] = tagRGB.rgbRed;
						pRowPointer[px++] = tagRGB.rgbGreen;
					}
					pRowPointer[px++]	= tagRGB.rgbBlue;
					pRowPointer[px]		= GetAlpha( ax, ay );
				}

				png_write_row( pPngPtr, pRowPointer );
				ay--;
			}
			else
#endif
			{
				Iter.GetRow( pRowPointer, nRowSize );
				if( pInfoPtr->color_type == PNG_COLOR_TYPE_RGB )
				{
					_RGBtoBGR( pRowPointer, nRowSize );
				}
				png_write_row( pPngPtr, pRowPointer );
			}
		}while( Iter.PrewRow() );
	}

	delete [] pRowPointer;

	if( !bIsGrayScale && m_tagHead.biClrUsed && m_tagInfo.m_lBkgndIndex > 0 )
	{
		SwapIndex( (BYTE)m_tagInfo.m_lBkgndIndex, 0 );
	}

	png_write_end( pPngPtr, pInfoPtr );

	if( pInfoPtr->palette )
	{
		delete [] (pInfoPtr->palette);
		pInfoPtr->palette = NULL;
	}

	png_destroy_write_struct( &pPngPtr, (png_infopp)&pInfoPtr );

	return true;
}
//-------------------------------------------------------------------------
bool CFKImagePNG::Encode( FILE* p_hFile )
{
	CFKIOFile file( p_hFile );
	return Encode( &file );
}
//-------------------------------------------------------------------------
void CFKImagePNG::_PngError( png_struct* p_pPngPtr, char* p_szMessage )
{
	strcpy( m_tagInfo.m_szLastError, p_szMessage );
	longjmp( p_pPngPtr->jmpbuf, 1 );
}
//-------------------------------------------------------------------------
void CFKImagePNG::_Expand2To4Bpp( BYTE* p_pRow )
{
	BYTE* pSrc = NULL;
	BYTE* pDst = NULL;
	BYTE byPos = 0;
	BYTE byIndex = 0;
	for( long x = m_tagHead.biWidth - 1; x >= 0; x-- )
	{
		pSrc = p_pRow + ( ( 2*x ) >> 3 );
		pDst = p_pRow + ( ( 4*x ) >> 3 );
		byPos = (BYTE)( 2 * ( 3 - x % 4 ) );
		byIndex = (BYTE)( (*pSrc & (0x03 << byPos)) >> byPos );
		byPos = (BYTE)( 4 * ( 1 - x % 2 ) );
		*pDst &= ~(0x0F << byPos);
		*pDst |= (byIndex & 0x0F) << byPos;
	}
}
//-------------------------------------------------------------------------
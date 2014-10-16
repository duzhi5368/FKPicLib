//-------------------------------------------------------------------------
#include "../Include/FKImageJpeg.h"
#include "../Include/FKIOFile.h"
#include "../Include/FKImageIterator.h"
#include "../Include/FKFileJpg.h"
#include <setjmp.h>
//-------------------------------------------------------------------------
struct SJpegErrorMgr
{
	struct jpeg_error_mgr	m_tagPub;
	jmp_buf					m_SetJmpBuffer;
	char*					m_szBuffer;
};
//-------------------------------------------------------------------------
enum CODEC_OPTION
{
	ENCODE_BASELINE = 0x1,
	ENCODE_ARITHMETIC = 0x2,
	ENCODE_GRAYSCALE = 0x4,
	ENCODE_OPTIMIZE = 0x8,
	ENCODE_PROGRESSIVE = 0x10,
	ENCODE_LOSSLESS = 0x20,
	ENCODE_SMOOTHING = 0x40,
	DECODE_GRAYSCALE = 0x80,
	DECODE_QUANTIZE = 0x100,
	DECODE_DITHER = 0x200,
	DECODE_ONEPASS = 0x400,
	DECODE_NOSMOOTH = 0x800,
	ENCODE_SUBSAMPLE_422 = 0x1000,
	ENCODE_SUBSAMPLE_444 = 0x2000
}; 
//-------------------------------------------------------------------------
CFKImageJPG::CFKImageJPG()
	: CFKImage( eFormats_JPG )
{
#if USE_FK_EXIF
	m_pExif = NULL;
	memset( &m_tagHead, 0, sizeof(SExifInfo) );
#endif
}
//-------------------------------------------------------------------------
CFKImageJPG::~CFKImageJPG()
{
#if USE_FK_EXIF
	if( m_pExif )
		delete m_pExif;
#endif
}
//-------------------------------------------------------------------------
bool CFKImageJPG::Decode( IFKFile* p_hFile )
{
	if( _IsHadFreeKnightSign(p_hFile) )
		return false;

	bool bIsExif = false;
#if USE_FK_EXIF
	bIsExif = DecodeExif( p_hFile );
#endif
	CFKImageIterator Iter( this );
	struct jpeg_decompress_struct	tagInfo;
	struct SJpegErrorMgr			tagJerr;
	tagJerr.m_szBuffer				= m_tagInfo.m_szLastError;
	JSAMPARRAY		szBuffer		= NULL;
	int				nRowStride		= 0;
	tagInfo.err						= jpeg_std_error(&tagJerr.m_tagPub);
	tagJerr.m_tagPub.error_exit		= JpegErrorExit;

	if( setjmp( tagJerr.m_SetJmpBuffer ) )
	{
		jpeg_destroy_decompress( &tagInfo );
		return NULL;
	}

	jpeg_create_decompress( &tagInfo );

	CFKFileJpg src( p_hFile );
	tagInfo.src = &src;

	(void)jpeg_read_header( &tagInfo, TRUE );

	if( (GetCodecOption( eFormats_JPG ) & DECODE_GRAYSCALE) != 0 )
		tagInfo.out_color_space = JCS_GRAYSCALE;
	if( (GetCodecOption( eFormats_JPG ) & DECODE_QUANTIZE) != 0 )
	{
		tagInfo.quantize_colors = TRUE;
		tagInfo.desired_number_of_colors = GetJpegQuality();
	}
	if( (GetCodecOption( eFormats_JPG ) & DECODE_DITHER) != 0 )
		tagInfo.dither_mode = m_nDither;
	if( (GetCodecOption( eFormats_JPG ) & DECODE_ONEPASS) != 0 )
		tagInfo.two_pass_quantize = FALSE;
	if( (GetCodecOption( eFormats_JPG ) & DECODE_NOSMOOTH) != 0 )
		tagInfo.do_fancy_upsampling = FALSE;

	tagInfo.scale_denom = GetJpegScale();

	if( m_tagInfo.m_lEscape == -1 )
	{
		jpeg_calc_output_dimensions( &tagInfo );
		m_tagHead.biWidth	= tagInfo.output_width;
		m_tagHead.biHeight	= tagInfo.output_height;
		m_tagInfo.m_dwType	= eFormats_JPG;
		jpeg_destroy_decompress( &tagInfo );
		return true;
	}

	jpeg_start_decompress( &tagInfo );

	Create( tagInfo.output_width, tagInfo.output_height, 8 * tagInfo.output_components, eFormats_JPG );
	if( !m_pDib )
	{
		longjmp( tagJerr.m_SetJmpBuffer, 1 );
	}

	if( bIsExif )
	{
#if USE_FK_EXIF
		if(( m_tagExifInfo.Xresolution != 0.0f ) && ( m_tagExifInfo.ResolutionUnit != 0 ))
			SetXDpi((long)(m_tagExifInfo.Xresolution / m_tagExifInfo.ResolutionUnit ));
		if(( m_tagExifInfo.Yresolution != 0.0f ) && ( m_tagExifInfo.ResolutionUnit != 0 ))
			SetYDpi((long)(m_tagExifInfo.Yresolution / m_tagExifInfo.ResolutionUnit ));
#endif
	}
	else
	{
		switch ( tagInfo.density_unit )
		{
		case 0:
			if(( tagInfo.Y_density > 0 ) && ( tagInfo.X_density > 0 ))
			{
				SetYDpi( (long)(GetXDpi() * ((float)(tagInfo.Y_density) / (float)(tagInfo.X_density))));
			}
			break;
		case 1:
			SetXDpi( (long)floor( tagInfo.X_density * 2.54 + 0.5 ) );
			SetYDpi( (long)floor( tagInfo.Y_density * 2.54 + 0.5 ) );			
			break;
		default:
			SetXDpi( tagInfo.X_density );
			SetYDpi( tagInfo.Y_density );
			break;
		}
	}

	if( tagInfo.out_color_space == JCS_GRAYSCALE )
	{
		SetGrayPalette();
		m_tagHead.biClrUsed = 256;
	}
	else
	{
		if( tagInfo.quantize_colors )
		{
			SetPalette( tagInfo.actual_number_of_colors, tagInfo.colormap[0],
				tagInfo.colormap[1], tagInfo.colormap[2] );
			m_tagHead.biClrUsed = tagInfo.actual_number_of_colors;
		}
		else
		{
			m_tagHead.biClrUsed = 0;
		}
	}

	nRowStride = tagInfo.output_width * tagInfo.output_components;

	szBuffer = (*tagInfo.mem->alloc_sarray)((j_common_ptr) &tagInfo, JPOOL_IMAGE, nRowStride, 1);
	Iter.Upset();

	while( tagInfo.output_scanline < tagInfo.output_height )
	{
		if( m_tagInfo.m_lEscape )
		{
			longjmp( tagJerr.m_SetJmpBuffer, 1 );
		}
		(void)jpeg_read_scanlines( &tagInfo, szBuffer, 1 );
		if( ( tagInfo.num_components == 4 ) && ( tagInfo.quantize_colors == FALSE ) )
		{
			BYTE k = 0;
			BYTE* pDest = NULL;
			BYTE* pSrc = NULL;
			pDest = Iter.GetRow();
			pSrc = szBuffer[0];
			for( long x3=0,x4=0; x3<(long)m_tagInfo.m_dwEffWidth && x4<nRowStride; x3+=3, x4+=4)
			{
				k=pSrc[x4+3];
				pDest[x3]	=(BYTE)((k * pSrc[x4+2])/255);
				pDest[x3+1]	=(BYTE)((k * pSrc[x4+1])/255);
				pDest[x3+2]	=(BYTE)((k * pSrc[x4+0])/255);
			}
		}
		else
		{
			Iter.SetRow( szBuffer[0], nRowStride );
		}
		Iter.PrewRow();
	}

	( void )jpeg_finish_decompress( &tagInfo );
	if( (tagInfo.num_components == 3) && (tagInfo.quantize_colors == FALSE) )
	{
		BYTE* pR0 = GetBits();
		for( long y = 0; y < m_tagHead.biHeight; ++y )
		{
			if( m_tagInfo.m_lEscape )
				longjmp( tagJerr.m_SetJmpBuffer, 1 );

			_RGBtoBGR( pR0, 3 * m_tagHead.biWidth );
			pR0 += m_tagInfo.m_dwEffWidth;
		}
	}

	jpeg_destroy_decompress( &tagInfo );

	return true;
}
//-------------------------------------------------------------------------
bool CFKImageJPG::Decode( FILE* p_hFile )
{
	CFKIOFile tagFile( p_hFile );
	return Decode( &tagFile );
}
//-------------------------------------------------------------------------
bool CFKImageJPG::Encode( IFKFile* p_hFile )
{
	if( _EncodeSafeCheck(p_hFile) )
		return false;

	if( m_tagHead.biClrUsed != NULL && !IsGrayScale() )
	{
		strcpy( m_tagInfo.m_szLastError,"JPEG can save only RGB or GreyScale images");
		return false;
	}

	long lPos = p_hFile->Tell();
	struct jpeg_compress_struct tagInfo;
	struct SJpegErrorMgr tagJerr;
	tagJerr.m_szBuffer = m_tagInfo.m_szLastError;
	int	nRowStride = 0;
	JSAMPARRAY pBuffer = NULL;
	tagInfo.err = jpeg_std_error(&tagJerr.m_tagPub);
	tagJerr.m_tagPub.error_exit = JpegErrorExit;
	if( setjmp( tagJerr.m_SetJmpBuffer ) )
	{
		strcpy( m_tagInfo.m_szLastError, tagJerr.m_szBuffer );
		jpeg_destroy_compress( &tagInfo );
		return false;
	}
	jpeg_create_compress(&tagInfo);
	CFKFileJpg tagDest( p_hFile );
	tagInfo.dest = &tagDest;
	tagInfo.image_width = GetWidth();
	tagInfo.image_height = GetHeight();
	if( IsGrayScale() )
	{
		tagInfo.input_components = 1;
		tagInfo.in_color_space = JCS_GRAYSCALE;
	}
	else
	{
		tagInfo.input_components = 3;
		tagInfo.in_color_space = JCS_RGB;
	}

	jpeg_set_defaults( &tagInfo );

	if(( GetCodecOption(eFormats_JPG) & ENCODE_ARITHMETIC ) != 0 )
		tagInfo.arith_code = TRUE;
	if(( GetCodecOption(eFormats_JPG) & ENCODE_OPTIMIZE ) != 0 )
		tagInfo.optimize_coding = TRUE;
	if(( GetCodecOption(eFormats_JPG) & ENCODE_GRAYSCALE ) != 0 )
		jpeg_set_colorspace( &tagInfo, JCS_GRAYSCALE );
	if(( GetCodecOption(eFormats_JPG) & ENCODE_SMOOTHING ) != 0 )
		tagInfo.smoothing_factor = 0;

	jpeg_set_quality(&tagInfo, GetJpegQuality(), ((GetCodecOption(eFormats_JPG) & ENCODE_BASELINE ) != 0) );

	if(( GetCodecOption(eFormats_JPG) & ENCODE_PROGRESSIVE ) != 0 )
		jpeg_simple_progression( &tagInfo );

	tagInfo.comp_info[0].h_samp_factor		= 2;
	tagInfo.comp_info[0].v_samp_factor		= 2;
	tagInfo.comp_info[1].h_samp_factor		= 1;
	tagInfo.comp_info[1].v_samp_factor		= 1;
	tagInfo.comp_info[2].h_samp_factor		= 1;
	tagInfo.comp_info[2].v_samp_factor		= 1;

	if( (GetCodecOption(eFormats_JPG) & ENCODE_SUBSAMPLE_422) != 0 )
	{
		tagInfo.comp_info[0].h_samp_factor		= 2;
		tagInfo.comp_info[0].v_samp_factor		= 1;
		tagInfo.comp_info[1].h_samp_factor		= 1;
		tagInfo.comp_info[1].v_samp_factor		= 1;
		tagInfo.comp_info[2].h_samp_factor		= 1;
		tagInfo.comp_info[2].v_samp_factor		= 1;
	}

	if( (GetCodecOption(eFormats_JPG) & ENCODE_SUBSAMPLE_444) != 0 )
	{
		tagInfo.comp_info[0].h_samp_factor		= 1;
		tagInfo.comp_info[0].v_samp_factor		= 1;
		tagInfo.comp_info[1].h_samp_factor		= 1;
		tagInfo.comp_info[1].v_samp_factor		= 1;
		tagInfo.comp_info[2].h_samp_factor		= 1;
		tagInfo.comp_info[2].v_samp_factor		= 1;
	}

	tagInfo.density_unit	= 1;
	tagInfo.X_density		= ( unsigned short )GetXDpi();
	tagInfo.Y_density		= ( unsigned short )GetYDpi();

	jpeg_start_compress( &tagInfo, TRUE );
	nRowStride = m_tagInfo.m_dwEffWidth;
	pBuffer = (*tagInfo.mem->alloc_sarray)((j_common_ptr) &tagInfo, JPOOL_IMAGE, 8+nRowStride, 1);

	CFKImageIterator Iter( this );
	Iter.Upset();

	while( tagInfo.next_scanline < tagInfo.image_height )
	{
		Iter.GetRow( pBuffer[0], nRowStride );
		if( m_tagHead.biClrUsed == 0 )
		{
			_RGBtoBGR( pBuffer[0], nRowStride );
		}
		Iter.PrewRow();
		(void)jpeg_write_scanlines(&tagInfo, pBuffer, 1);
	}
	jpeg_finish_compress(&tagInfo);
	jpeg_destroy_compress(&tagInfo);

#if USE_FK_EXIF
	if( m_pExif && m_pExif->m_pExifInfo->IsExif )
	{
		m_pExif->DiscardAllButExif();
		p_hFile->Seek( lPos, SEEK_SET );
		m_pExif->DecodeExif( p_hFile, eExifReadImage );
		p_hFile->Seek( lPos, SEEK_SET );
		m_pExif->EncodeExif( p_hFile );
	}
#endif
	return true;
}
//-------------------------------------------------------------------------
bool CFKImageJPG::Encode( FILE* p_hFile )
{
	CFKIOFile tagFile( p_hFile );
	return Encode( &tagFile );
}
//-------------------------------------------------------------------------
void CFKImageJPG::JpegErrorExit( j_common_ptr p_tagInfo )
{
	SJpegErrorMgr* pMyErr = (SJpegErrorMgr*)( p_tagInfo->err );
	pMyErr->m_tagPub.format_message( p_tagInfo, pMyErr->m_szBuffer );
	longjmp( pMyErr->m_SetJmpBuffer, 1 );
}
//-------------------------------------------------------------------------
#if USE_FK_EXIF
//-------------------------------------------------------------------------
bool CFKImageJPG::DecodeExif( IFKFile* p_hFile )
{
	m_pExif = new CFKExifInfo( &m_tagExifInfo );
	if( m_pExif )
	{
		long lPos = p_hFile->Tell();
		m_pExif->DecodeExif( p_hFile );
		p_hFile->Seek( lPos, SEEK_SET );
		return m_pExif->m_pExifInfo->IsExif;
	}
	else
	{
		return false;
	}
}
//-------------------------------------------------------------------------
bool CFKImageJPG::DecodeExif( FILE* p_hFile )
{
	CFKIOFile tagFile( p_hFile );
	return DecodeExif( &tagFile );
}
//-------------------------------------------------------------------------
#endif
//-------------------------------------------------------------------------
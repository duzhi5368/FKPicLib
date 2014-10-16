//*************************************************************************
//	��������:	2014-9-16   9:36
//	�ļ�����:	FKImageJpeg.h
//  �� �� ��:   ������ FreeKnight	
//	��Ȩ����:	MIT
//	˵    ��:	
//*************************************************************************

#pragma once

//-------------------------------------------------------------------------
#include "FKImage.h"
#include "FKImageExifInfo.h"
extern "C"
{
#include "../../JPEG/jpeglib.h"
#include "../../JPEG/jerror.h"
};
//-------------------------------------------------------------------------
class CFKImageJPG : public CFKImage
{
public:
	CFKImageJPG();
	~CFKImageJPG();
public:
	bool	Decode( IFKFile* p_hFile );
	bool	Decode( FILE* p_hFile );

	bool	Encode( IFKFile* p_hFile );
	bool	Encode( FILE* p_hFile );
public:
	static void JpegErrorExit( j_common_ptr p_tagInfo );
#if USE_FK_EXIF
	bool	DecodeExif( IFKFile* p_hFile );
	bool	DecodeExif( FILE* p_hFile );
public:
	CFKExifInfo*		m_pExif;
	SExifInfo			m_tagExifInfo;
#endif
	J_DITHER_MODE		m_nDither;
};
//-------------------------------------------------------------------------
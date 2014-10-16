//*************************************************************************
//	��������:	2014-9-18   10:24
//	�ļ�����:	FKImagePng.h
//  �� �� ��:   ������ FreeKnight	
//	��Ȩ����:	MIT
//	˵    ��:	
//*************************************************************************

#pragma once

//-------------------------------------------------------------------------
#include "FKImage.h"
#include "FKFile.h"
extern "C"
{
#include "../../PNG/png.h"
};
//-------------------------------------------------------------------------
class CFKImagePNG : public CFKImage
{
public:
	CFKImagePNG();
	~CFKImagePNG();
public:
	bool	Decode( IFKFile* p_hFile );
	bool	Decode( FILE* p_hFile );

	bool	Encode( IFKFile* p_hFile );
	bool	Encode( FILE* p_hFile );
protected:
	void	_PngError( png_struct* p_pPngPtr, char* p_szMessage );
	void	_Expand2To4Bpp( BYTE* p_pRow );
protected:
	static void PNGAPI MyReadDataFunc( png_structp p_pPngPtr, png_bytep p_pData, png_size_t p_unLength )
	{
		IFKFile* pFile = ( IFKFile* )png_get_io_ptr( p_pPngPtr );
		if( pFile == NULL || pFile->Read( p_pData, 1, p_unLength ) != p_unLength )
			png_error( p_pPngPtr, "Read error" );
	}
	static void PNGAPI MyErrorFunc( png_structp p_pPngPtr, png_const_charp p_szErrorMsg )
	{
		strncpy((char*)p_pPngPtr->error_ptr, p_szErrorMsg, 255 );
		longjmp(p_pPngPtr->jmpbuf, 1);
	}
	static void PNGAPI MyWriteDataFunc( png_structp p_pPngPtr, png_bytep p_pData, png_size_t p_unLength )
	{
		IFKFile* pFile = ( IFKFile* )png_get_io_ptr( p_pPngPtr );
		if( pFile == NULL || pFile->Write( p_pData, 1, p_unLength ) != p_unLength )
			png_error( p_pPngPtr, "Write error" );
	}
	static void PNGAPI MyFlushDataFunc( png_structp p_pPngPtr )
	{
		IFKFile* pFile = ( IFKFile* )png_get_io_ptr( p_pPngPtr );
		if( pFile == NULL || !pFile->Flush() )
			png_error( p_pPngPtr, "Flush error" );
	}
};
//-------------------------------------------------------------------------
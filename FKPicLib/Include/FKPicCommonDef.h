//*************************************************************************
//	��������:	2014-9-11   9:26
//	�ļ�����:	FKPicCommonDef.h
//  �� �� ��:   ������ FreeKnight	
//	��Ȩ����:	MIT
//	˵    ��:	
//*************************************************************************

#pragma once

//-------------------------------------------------------------------------
// ����ͷ
//-------------------------------------------------------------------------
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tchar.h>
//-------------------------------------------------------------------------
// ȥ������
//-------------------------------------------------------------------------
#pragma warning( disable : 4996 )
//-------------------------------------------------------------------------
// ϵ�к꿪��
//-------------------------------------------------------------------------
// ����/�ر�ѡ����
#define USE_FK_SELECTION		1
// ����/�ر�Alphaͨ��
#define USE_FK_ALPHA			1

// �Ƿ�֧��Exif��ʽ����ʵ����Jpegǰ��ӵ�����������Ϣ��
#define USE_FK_EXIF				1
//-------------------------------------------------------------------------
// ����ֵ
//-------------------------------------------------------------------------
// Ĭ��DPI
const static unsigned int	DEFAULT_DPI			= 96;
// ���λImage�ڴ�
const static unsigned int	IMAGE_MAX_MEMORY	= 0x10000000;

// ѹ������
enum ENUM_COMPRESSION_TYPE
{
	eBI_RGB			= 0L,
	eBI_RLE8 		= 1L,     
	eBI_RLE4    	= 2L,  
	eBI_BITFIELDS 	= 3L,
};

// ��ǰ֧�ֵ�ͼ�θ�ʽ
enum ENUM_FKPIC_FORMATS
{
	eFormats_Unknown = 0,

	eFormats_BMP	= 1,
	eFormats_JPG	= 2,
	eFormats_PNG	= 3,
	eFormats_FKP	= 4,

	eFormats_Max,
};
//-------------------------------------------------------------------------
// ������
//-------------------------------------------------------------------------
typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;
typedef unsigned int		UINT;

typedef DWORD				COLORREF;
//typedef unsigned int		HANDLE;

#ifndef BOOL
#define	BOOL bool
#endif

#ifndef TRUE
#define	TRUE true
#endif

#ifndef FALSE
#define	FALSE false
#endif

#ifndef TCHAR
#define TCHAR char
#endif

#ifndef _T
#define _T
#endif

#ifndef PI
#define PI 3.141592653589793f
#endif
//-------------------------------------------------------------------------
// ����֧�ֺ���
//-------------------------------------------------------------------------
// RGBת���Ҷ�ͼ
#ifndef RGB2GAY
#define RGB2GAY(r,g,b) (((b)*117 + (g)*601 + (r)*306) >> 10)
#endif

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
//-------------------------------------------------------------------------
// �����ṹ
//-------------------------------------------------------------------------
/*typedef*/ struct SRGBQuad {
	BYTE    rgbBlue;
	BYTE    rgbGreen;
	BYTE    rgbRed;
	BYTE    rgbReserved;
} /*RGBQUAD*/;

typedef struct SRGBColor 
{ 
	BYTE r,g,b; 
} RGBCOLOR;

/*typedef*/ struct SRect
{
	long    left;
	long    top;
	long    right;
	long    bottom;
} /*RECT*/;

#pragma pack(1)

/*typedef*/ struct SBitmapInfoHeader{
	DWORD		biSize;
	long		biWidth;
	long		biHeight;
	WORD		biPlanes;
	WORD		biBitCount;
	DWORD		biCompression;
	DWORD		biSizeImage;
	long		biXPelsPerMeter;
	long		biYPelsPerMeter;
	DWORD		biClrUsed;
	DWORD		biClrImportant;
} /*BITMAPINFOHEADER*/;

/*typedef*/ struct SBitmapFileHeader {
	WORD		bfType;
	DWORD		bfSize;
	WORD		bfReserved1;
	WORD		bfReserved2;
	DWORD		bfOffBits;
} /*BITMAPFILEHEADER*/;

/*typedef*/ struct SBitmapCoreHeader {
	DWORD		bcSize;
	WORD		bcWidth;
	WORD		bcHeight;
	WORD		bcPlanes;
	WORD		bcBitCount;
} /*BITMAPCOREHEADER*/;

/*typedef*/ struct SRgbTriple {
	BYTE		rgbtBlue;
	BYTE		rgbtGreen;
	BYTE		rgbtRed;
} /*RGBTRIPLE*/;

#pragma pack()
//-------------------------------------------------------------------------
// ����������Ϣ
//-------------------------------------------------------------------------
#define ERR_NO_FILE			"empty file handler"
#define ERR_NO_IMAGE		"empty image"
//-------------------------------------------------------------------------
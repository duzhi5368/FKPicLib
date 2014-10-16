//*************************************************************************
//	��������:	2014-9-16   9:43
//	�ļ�����:	FKFileJpg.h
//  �� �� ��:   ������ FreeKnight	
//	��Ȩ����:	MIT
//	˵    ��:	
//*************************************************************************

#pragma once

//-------------------------------------------------------------------------
#include "FKFile.h"
extern "C"
{
#include "../../JPEG/jpeglib.h"
#include "../../JPEG/jerror.h"
};
//-------------------------------------------------------------------------
// Ĭ���ļ������С
const static int JEPG_FILE_BUF_SIZE		= 4096;
//-------------------------------------------------------------------------
class CFKFileJpg : public jpeg_destination_mgr, public jpeg_source_mgr
{
public:
	CFKFileJpg( IFKFile* p_pFile );
	~CFKFileJpg();
public:
	static void InitDestination( j_compress_ptr p_pInfo );
	static boolean EmptyOutputBuffer( j_compress_ptr p_pInfo );
	static void	TermDestination( j_compress_ptr p_pInfo );
	static void InitSource( j_decompress_ptr p_pInfo );
	static boolean	FillInputBuffer( j_decompress_ptr p_pInfo );
	static void SkipInputData( j_decompress_ptr p_pInfo, long p_lNumBytes );
	static void TermSource( j_decompress_ptr p_pInfo );
protected:
	IFKFile*		m_pFile;
	unsigned char*	m_pBuffer;
	bool			m_bIsStartOfFile;
};
//-------------------------------------------------------------------------
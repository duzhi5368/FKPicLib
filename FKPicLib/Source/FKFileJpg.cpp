//-------------------------------------------------------------------------
#include "../Include/FKPicCommonDef.h"
#include "../Include/FKFileJpg.h"
//-------------------------------------------------------------------------
CFKFileJpg::CFKFileJpg( IFKFile* p_pFile )
{
	m_pFile = p_pFile;
	m_pBuffer = new unsigned char[JEPG_FILE_BUF_SIZE];
	init_destination		= InitDestination;
	empty_output_buffer		= EmptyOutputBuffer;
	term_destination		= TermDestination;
	init_source				= InitSource;
	fill_input_buffer		= FillInputBuffer;
	skip_input_data			= SkipInputData;
	resync_to_restart		= jpeg_resync_to_restart;
	term_source				= TermSource;
	next_input_byte			= NULL;		// 读入缓冲区的下一字节
	bytes_in_buffer			= 0;
	m_bIsStartOfFile		= false;
}
//-------------------------------------------------------------------------
CFKFileJpg::~CFKFileJpg()
{
	delete [] m_pBuffer;
}
//-------------------------------------------------------------------------
void CFKFileJpg::InitDestination( j_compress_ptr p_pInfo )
{
	CFKFileJpg* pDest = ( CFKFileJpg* )p_pInfo->dest;
	pDest->next_output_byte = pDest->m_pBuffer;
	pDest->free_in_buffer	= JEPG_FILE_BUF_SIZE;
}
//-------------------------------------------------------------------------
boolean CFKFileJpg::EmptyOutputBuffer( j_compress_ptr p_pInfo )
{
	CFKFileJpg* pDest = ( CFKFileJpg* )p_pInfo->dest;
	if( pDest->m_pFile->Write(pDest->m_pBuffer, 1, JEPG_FILE_BUF_SIZE ) != (size_t)JEPG_FILE_BUF_SIZE )
		ERREXIT( p_pInfo, JERR_FILE_WRITE );
	pDest->next_output_byte	= pDest->m_pBuffer;
	pDest->free_in_buffer	= JEPG_FILE_BUF_SIZE;
	return true;
}
//-------------------------------------------------------------------------
void CFKFileJpg::TermDestination( j_compress_ptr p_pInfo )
{
	CFKFileJpg* pDest = ( CFKFileJpg* )p_pInfo->dest;
	size_t unDataCount = JEPG_FILE_BUF_SIZE - pDest->free_in_buffer;
	if( unDataCount > 0 )
	{
		if( !pDest->m_pFile->Write( pDest->m_pBuffer, 1, unDataCount ))
			ERREXIT( p_pInfo, JERR_FILE_WRITE );
	}
	pDest->m_pFile->Flush();
	if( pDest->m_pFile->Error())
		ERREXIT( p_pInfo, JERR_FILE_WRITE );
	return;
}
//-------------------------------------------------------------------------
void CFKFileJpg::InitSource( j_decompress_ptr p_pInfo )
{
	CFKFileJpg* pSrc = ( CFKFileJpg* )p_pInfo->src;
	pSrc->m_bIsStartOfFile = true;
}
//-------------------------------------------------------------------------
boolean CFKFileJpg::FillInputBuffer( j_decompress_ptr p_pInfo )
{
	size_t nBytes = 0;
	CFKFileJpg* pSource = (CFKFileJpg*)p_pInfo->src;
	nBytes = pSource->m_pFile->Read( pSource->m_pBuffer, 1, JEPG_FILE_BUF_SIZE );
	if( nBytes <= 0 )
	{
		if( pSource->m_bIsStartOfFile )
			ERREXIT( p_pInfo, JERR_INPUT_EMPTY );
		WARNMS( p_pInfo, JWRN_JPEG_EOF );

		pSource->m_pBuffer[0] = (JOCTET)0xFF;
		pSource->m_pBuffer[1] = (JOCTET)JPEG_EOI;
		nBytes = 2;
	}
	pSource->next_input_byte		= pSource->m_pBuffer;
	pSource->bytes_in_buffer		= nBytes;
	pSource->m_bIsStartOfFile		= false;
	return true;
}
//-------------------------------------------------------------------------
void CFKFileJpg::SkipInputData( j_decompress_ptr p_pInfo, long p_lNumBytes )
{
	CFKFileJpg* pSource = ( CFKFileJpg* )p_pInfo->src;
	if( p_lNumBytes > 0 )
	{
		while( p_lNumBytes > ( long )pSource->bytes_in_buffer )
		{
			p_lNumBytes -= ( long )pSource->bytes_in_buffer;
			FillInputBuffer( p_pInfo );
		}
		pSource->next_input_byte	+= (size_t)p_lNumBytes;
		pSource->bytes_in_buffer	-= (size_t)p_lNumBytes;
	}
}
//-------------------------------------------------------------------------
void CFKFileJpg::TermSource( j_decompress_ptr p_pInfo )
{
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#include "../Include/FKIOFile.h"
//-------------------------------------------------------------------------
CFKIOFile::CFKIOFile( FILE* p_pFile )
{
	m_pFile = p_pFile;
	m_bCloseFile = ( m_pFile == NULL );
}
//-------------------------------------------------------------------------
CFKIOFile::~CFKIOFile()
{
	Close();
}
//-------------------------------------------------------------------------
bool CFKIOFile::Open( const char* p_szFileName, const char* p_szMode )
{
	if( m_pFile )
		return false;
	m_pFile = _tfopen( p_szFileName, p_szMode );
	if( m_pFile )
		return false;
	m_bCloseFile = true;
	return true;
}
//-------------------------------------------------------------------------
bool CFKIOFile::Close()
{
	int nErr = 0;
	if ( ( m_pFile ) && ( m_bCloseFile ) ){ 
		nErr = fclose( m_pFile );
		m_pFile = NULL;
	}
	return ( nErr == 0 );
}
//-------------------------------------------------------------------------
size_t CFKIOFile::Read(void* p_szBuffer, size_t p_unSize, size_t p_unCount)
{
	if ( m_pFile == NULL  ) 
		return 0;
	return fread(p_szBuffer, p_unSize, p_unCount, m_pFile);
}
//-------------------------------------------------------------------------
size_t CFKIOFile::Write(const void* p_szBuffer, size_t p_unSize, size_t p_unCount)
{
	if ( m_pFile == NULL  ) 
		return 0;
	return fwrite(p_szBuffer, p_unSize, p_unCount, m_pFile);
}
//-------------------------------------------------------------------------
bool CFKIOFile::Seek(long p_lOffset, int p_nOrigin)
{
	if ( m_pFile == NULL  ) 
		return false;
	return ( fseek(m_pFile, p_lOffset, p_nOrigin) == 0 );
}
//-------------------------------------------------------------------------
long CFKIOFile::Tell()
{
	if ( m_pFile == NULL  ) 
		return 0;
	return ftell( m_pFile );
}
//-------------------------------------------------------------------------
long CFKIOFile::Size()
{
	if ( m_pFile == NULL  ) 
		return -1;
	long lPos,lSize;
	lPos = ftell( m_pFile );
	fseek( m_pFile, 0, SEEK_END );
	lSize = ftell( m_pFile );
	fseek( m_pFile, lPos, SEEK_SET );
	return lSize;
}
//-------------------------------------------------------------------------
bool CFKIOFile::Flush()
{
	if ( m_pFile == NULL  ) 
		return false;
	return ( fflush( m_pFile ) == 0 );
}
//-------------------------------------------------------------------------
bool CFKIOFile::Eof()
{
	if ( m_pFile == NULL  ) 
		return true;
	return ( feof( m_pFile ) != 0 );
}
//-------------------------------------------------------------------------
long CFKIOFile::Error()
{
	if ( m_pFile == NULL  ) 
		return EOF;
	return ferror( m_pFile );
}
//-------------------------------------------------------------------------
long CFKIOFile::GetC()
{
	if ( m_pFile == NULL  ) 
		return EOF;
	return getc( m_pFile );
}
//-------------------------------------------------------------------------
char* CFKIOFile::GetS(char* p_szString, int p_nN )
{
	if ( m_pFile == NULL  ) 
		return NULL;
	return fgets( p_szString, p_nN, m_pFile );
}
//-------------------------------------------------------------------------
long CFKIOFile::Scanf(const char* p_szFormat, void* p_pOutput)
{
	if ( m_pFile == NULL  ) 
		return EOF;
	return fscanf( m_pFile, p_szFormat, p_pOutput );
}
//-------------------------------------------------------------------------
bool CFKIOFile::PutC(unsigned char p_ucChar)
{
	if ( m_pFile == NULL  ) 
		return false;
	return ( fputc( p_ucChar, m_pFile) == p_ucChar );
}
//-------------------------------------------------------------------------
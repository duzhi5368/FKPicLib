//-------------------------------------------------------------------------
#include "../Include/FKMemFile.h"
//-------------------------------------------------------------------------
CFKMemFile::CFKMemFile( BYTE* p_byBuf, DWORD p_dwSize )
{
	m_pBuffer = p_byBuf;
	m_lPosition = 0;
	m_dwSize = m_lBufferSize = p_dwSize;
	m_bIsFreeOnClose = ( p_byBuf == 0 );
}
//-------------------------------------------------------------------------
CFKMemFile::~CFKMemFile()
{
	Close();
}
//-------------------------------------------------------------------------
bool CFKMemFile::Open()
{
	if( m_pBuffer )
		return false;

	m_lPosition = m_lBufferSize = m_dwSize = 0;
	m_pBuffer = ( BYTE* )malloc( 1 );
	m_bIsFreeOnClose = true;
	return ( m_pBuffer != NULL );
}
//-------------------------------------------------------------------------
BYTE* CFKMemFile::GetBuffer( bool p_bDetachBuffer )
{
	if( p_bDetachBuffer )
		m_bIsFreeOnClose = false;

	return m_pBuffer;
}
//-------------------------------------------------------------------------
bool CFKMemFile::Close()
{
	if( m_pBuffer && m_bIsFreeOnClose )
	{
		free( m_pBuffer );
		m_pBuffer = NULL;
		m_dwSize = 0;
	}
	return true;
}
//-------------------------------------------------------------------------
size_t CFKMemFile::Read(void* p_szBuffer, size_t p_unSize, size_t p_unCount)
{
	if( p_szBuffer == NULL )
		return 0;
	if( m_pBuffer == NULL )
		return 0;
	if( m_lPosition >= static_cast<long>(m_dwSize) )
		return 0;
	long lCount = static_cast<long>( p_unCount * p_unSize );
	if( lCount == 0 )
		return 0;

	long lRead = 0;
	if( m_lPosition + lCount > static_cast<long>(m_dwSize) )
	{
		lRead = ( m_dwSize - m_lPosition );
	}
	else
	{
		lRead = lCount;
	}

	memcpy( p_szBuffer, m_pBuffer + m_lPosition, lRead );
	m_lPosition += lRead;

	return static_cast<size_t>( lRead / p_unSize );
}
//-------------------------------------------------------------------------
size_t CFKMemFile::Write(const void* p_szBuffer, size_t p_unSize, size_t p_unCount)
{
	if( m_pBuffer == NULL )
		return 0;
	if( p_szBuffer == NULL )
		return 0;
	long lCount = static_cast<long>( p_unCount * p_unSize );
	if( lCount == 0 )
		return 0;

	if( m_lPosition + static_cast<long>( p_unCount ) > m_lBufferSize )
	{
		if( !_Alloc( m_lPosition + p_unCount ) )
		{
			return false;
		}
	}

	memcpy( m_pBuffer + m_lPosition, p_szBuffer, lCount );
	m_lPosition += lCount;

	if( m_lPosition > static_cast<long>(m_dwSize) )
	{
		m_dwSize = m_lPosition;
	}
	return p_unCount;
}
//-------------------------------------------------------------------------
bool CFKMemFile::Seek(long p_lOffset, int p_nOrigin)
{
	if( m_pBuffer == NULL )
		return false;

	long lNewPos = m_lPosition;
	if( p_nOrigin == SEEK_SET )
		lNewPos = p_lOffset;
	else if( p_nOrigin == SEEK_CUR )
		lNewPos += p_lOffset;
	else if( p_nOrigin == SEEK_END )
		lNewPos = m_dwSize + p_lOffset;
	else
	{
		return false;
	}

	if( lNewPos < 0 )
		lNewPos = 0;

	m_lPosition = lNewPos;
	return true;
}
//-------------------------------------------------------------------------
long CFKMemFile::Tell()
{
	if( m_pBuffer == NULL )
		return -1;
	return m_lPosition;
}
//-------------------------------------------------------------------------
long CFKMemFile::Size()
{
	if( m_pBuffer == NULL )
		return -1;
	return static_cast<long>(m_dwSize);
}
//-------------------------------------------------------------------------
bool CFKMemFile::Flush()
{
	if( m_pBuffer == NULL )
		return false;
	return true;
}
//-------------------------------------------------------------------------
bool CFKMemFile::Eof()
{
	if( m_pBuffer == NULL )
		return true;
	return ( m_lPosition >= static_cast<long>(m_dwSize) );
}
//-------------------------------------------------------------------------
long CFKMemFile::Error()
{
	if( m_pBuffer == NULL )
		return -1;

	return ( m_lPosition > static_cast<long>(m_dwSize) );
}
//-------------------------------------------------------------------------
long CFKMemFile::GetC()
{
	if( Eof() )
		return EOF;
	return *( BYTE* )( (BYTE*)m_pBuffer + m_lPosition++ );
}
//-------------------------------------------------------------------------
char* CFKMemFile::GetS(char* p_szString, int p_nN )
{
	p_nN--;
	long c = 0;
	long i = 0;
	while( i < c )
	{
		c = GetC();
		if( c == EOF )
			return NULL;
		p_szString[i++] = static_cast<char>( c );
		if( c == '\n' )
			break;
	}
	p_szString[i] = 0;
	return p_szString;
}
//-------------------------------------------------------------------------
long CFKMemFile::Scanf(const char* p_szFormat, void* p_pOutput)
{
	return 0;
}
//-------------------------------------------------------------------------
bool CFKMemFile::PutC(unsigned char p_ucChar)
{
	if( m_pBuffer == NULL )
		return false;

	if( m_lPosition >= m_lBufferSize )
	{
		if( !_Alloc( m_lPosition + 1 ) )
		{
			return false;
		}
	}

	m_pBuffer[m_lPosition++] = p_ucChar;

	if( m_lPosition > static_cast<long>(m_dwSize) )
		m_dwSize = m_lPosition;

	return true;
}
//-------------------------------------------------------------------------
bool CFKMemFile::_Alloc( DWORD p_dwBytes )
{
	if( p_dwBytes > static_cast<DWORD>(m_lBufferSize) )
	{
		// 获取新内存大小
		DWORD dwNewBufSize = ( DWORD )( ( (p_dwBytes >> 16) + 1 ) << 16 );

		// 分配新内存
		if( m_pBuffer == NULL )
		{
			m_pBuffer = ( BYTE* )malloc( dwNewBufSize );
		}
		else
		{
			m_pBuffer = ( BYTE* )realloc( m_pBuffer, dwNewBufSize );
		}

		m_bIsFreeOnClose = true;
		m_lBufferSize = dwNewBufSize;
	}
	return ( m_pBuffer != NULL );
}
//-------------------------------------------------------------------------
void CFKMemFile::_Free()
{
	Close();
}
//-------------------------------------------------------------------------
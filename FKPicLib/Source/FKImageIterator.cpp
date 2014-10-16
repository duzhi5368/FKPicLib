//-------------------------------------------------------------------------
#include "../Include/FKImageIterator.h"
//-------------------------------------------------------------------------
CFKImageIterator::CFKImageIterator()
{
	m_pImage = NULL;
	m_pIterImage = NULL;
	m_nItY = m_nItX = m_nStepX = m_nStepY = 0;
}
//-------------------------------------------------------------------------
CFKImageIterator::CFKImageIterator( CFKImage* p_pImage )
{
	if( p_pImage )
		m_pIterImage = p_pImage->GetBits();
	m_pImage = p_pImage;
	m_nItY = m_nItX = m_nStepX = m_nStepY = 0;
}
//-------------------------------------------------------------------------
CFKImageIterator::operator CFKImage*()
{
	return m_pImage;
}
//-------------------------------------------------------------------------
CFKImageIterator::~CFKImageIterator()
{
	m_pImage = NULL;
	m_pIterImage = NULL;
	m_nItY = m_nItX = m_nStepX = m_nStepY = 0;
}
//-------------------------------------------------------------------------
bool CFKImageIterator::ItOK()
{
	if( m_pImage )
		return m_pImage->IsInside( m_nItX, m_nItY );
	else
	{
		return false;
	}
}
//-------------------------------------------------------------------------
void CFKImageIterator::Reset()
{
	if( m_pImage )
	{
		m_pIterImage = m_pImage->GetBits();
	}
	else
	{
		m_pIterImage = NULL;
	}
	m_nItY = m_nItX = m_nStepX = m_nStepY = 0;
}
//-------------------------------------------------------------------------
void CFKImageIterator::Upset()
{
	m_nItX = 0;
	m_nItY = m_pImage->GetHeight() - 1;
	m_pIterImage = m_pImage->GetBits() + m_pImage->GetEffWidth() * ( m_pImage->GetHeight() - 1 );
}
//-------------------------------------------------------------------------
bool CFKImageIterator::NextRow()
{
	if( ++m_nItY >= ( int )(m_pImage->GetHeight() ) )
		return false;
	m_pIterImage += m_pImage->GetEffWidth();
	return true;
}
//-------------------------------------------------------------------------
bool CFKImageIterator::PrewRow()
{
	if( --m_nItY < 0 )
		return false;
	m_pIterImage -= m_pImage->GetEffWidth();
	return true;
}
//-------------------------------------------------------------------------
void CFKImageIterator::SetY( int p_nY )
{
	if( ( p_nY < 0 ) || ( p_nY > (int)(m_pImage->GetHeight() ) ) )
		return;

	m_nItY = p_nY;
	m_pIterImage = m_pImage->GetBits() + m_pImage->GetEffWidth() * p_nY;
}
//-------------------------------------------------------------------------
void CFKImageIterator::SetRow( BYTE* p_byBuf, int p_nN )
{
	if( p_nN < 0 )
	{
		p_nN = ( int )m_pImage->GetEffWidth();
	}
	else
	{
		p_nN = min( p_nN, (int)m_pImage->GetEffWidth() );
	}
	if( m_pIterImage != NULL && p_byBuf != NULL && p_nN > 0 )
		memcpy( m_pIterImage, p_byBuf, p_nN );
}
//-------------------------------------------------------------------------
void CFKImageIterator::GetRow( BYTE* p_byBuf, int p_nN )
{
	if( m_pIterImage != NULL && p_byBuf != NULL && p_nN > 0 )
		memcpy( p_byBuf, m_pIterImage, min(p_nN, (int)m_pImage->GetEffWidth()) );
}
//-------------------------------------------------------------------------
BYTE* CFKImageIterator::GetRow()
{
	return m_pIterImage;
}
//-------------------------------------------------------------------------
BYTE* CFKImageIterator::GetRow( int p_nN )
{
	SetY( p_nN );
	return m_pIterImage;
}
//-------------------------------------------------------------------------
bool CFKImageIterator::NextByte()
{
	if( ++m_nItX < (int)(m_pImage->GetEffWidth() ) )
		return true;
	else
	{
		if( ++m_nItY < (int)(m_pImage->GetHeight()) )
		{
			m_pIterImage += m_pImage->GetEffWidth();
			m_nItX = 0;
			return true;
		}
		else
		{
			return false;
		}
	}
}
//-------------------------------------------------------------------------
bool CFKImageIterator::PrewByte()
{
	if( --m_nItX >= 0 )
		return true;
	else
	{
		if( --m_nItY >= 0 )
		{
			m_pIterImage -= m_pImage->GetEffWidth();
			m_nItX = 0;
			return true;
		}
		else
		{
			return false;
		}
	}
}
//-------------------------------------------------------------------------
bool CFKImageIterator::NextStep()
{
	m_nItX += m_nStepX;
	if( m_nItX < ( int )(m_pImage->GetEffWidth() ) )
		return true;
	else
	{
		m_nItY += m_nStepY;
		if( m_nItY < (int)(m_pImage->GetHeight() ) )
		{
			m_pIterImage += m_pImage->GetEffWidth();
			m_nItX = 0;
			return true;
		}
		else
		{
			return false;
		}
	}
}
//-------------------------------------------------------------------------
bool CFKImageIterator::PrewStep()
{
	m_nItX -= m_nStepX;
	if( m_nItX >= 0 )
		return true;
	else
	{
		m_nItY -= m_nStepY;
		if( m_nItY >= 0 && m_nItY < (int)(m_pImage->GetHeight()) )
		{
			m_pIterImage -= m_pImage->GetEffWidth();
			m_nItX = 0;
			return true;
		}
		else
		{
			return false;
		}
	}
}
//-------------------------------------------------------------------------
bool CFKImageIterator::GetCol( BYTE* p_pCol, DWORD p_dwX )
{
	if( p_pCol == NULL || m_pImage->GetBpp() < 8 || p_dwX >= m_pImage->GetWidth() )
		return false;

	DWORD	dwHeight = m_pImage->GetHeight();
	BYTE	byBytes = (BYTE)(m_pImage->GetBpp() >> 3);
	BYTE*	pSrc = NULL;
	for( DWORD dwY = 0; dwY < dwHeight; ++dwY )
	{
		pSrc = m_pImage->GetBits( dwY ) + p_dwX * byBytes;
		for( BYTE w = 0; w < byBytes; ++w )
		{
			*p_pCol++ = *pSrc++;
		}
	}
	return true;
}
//-------------------------------------------------------------------------
bool CFKImageIterator::SetCol( BYTE* p_pCol, DWORD p_dwX )
{
	if( p_pCol == NULL || m_pImage->GetBpp() < 8 || p_dwX >= m_pImage->GetWidth() )
		return false;

	DWORD	dwHeight = m_pImage->GetHeight();
	BYTE	byBytes = (BYTE)(m_pImage->GetBpp() >> 3);
	BYTE*	pSrc = NULL;
	for( DWORD dwY = 0; dwY < dwHeight; ++dwY )
	{
		pSrc = m_pImage->GetBits( dwY ) + p_dwX * byBytes;
		for( BYTE w = 0; w < byBytes; ++w )
		{
			*pSrc++ = *p_pCol++;
		}
	}
	return true;
}
//-------------------------------------------------------------------------
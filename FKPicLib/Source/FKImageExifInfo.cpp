//-------------------------------------------------------------------------
#include "../Include/FKImageExifInfo.h"
//-------------------------------------------------------------------------
#if USE_FK_EXIF
//-------------------------------------------------------------------------
CFKExifInfo::CFKExifInfo( SExifInfo* p_pInfo )
{
	if( p_pInfo )
	{
		m_pExifInfo = p_pInfo;
		m_bIsFreeInfo = false;
	}
	else
	{
		m_pExifInfo = new SExifInfo();
		memset( m_pExifInfo, 0, sizeof( SExifInfo ) );
		m_bIsFreeInfo = true;
	}
	m_szLastError[0] = '\0';
	m_nExifImageWidth = m_nMotorolaOrder = m_nSectionsRead = 0;
	memset( &m_vecSections, 0, EXIF_MAX_SECTIONS * sizeof(SSection) );
}
//-------------------------------------------------------------------------
CFKExifInfo::~CFKExifInfo()
{
	for( int i = 0; i < EXIF_MAX_SECTIONS; ++i )
	{
		if( m_vecSections[i].Data )
		{
			free( m_vecSections[i].Data );
		}
	}
	if( m_bIsFreeInfo )
	{
		delete m_pExifInfo;
		m_pExifInfo = NULL;
	}
}
//-------------------------------------------------------------------------
bool CFKExifInfo::DecodeExif( IFKFile* p_hFile, int p_nReadMode )
{
	int a = 0;
	bool bIsHaveCom = false;
	a = p_hFile->GetC();
	if( a != 0xff || p_hFile->GetC() != M_SOI )
	{
		return false;
	}
	for( ; ; )
	{
		int nItemLen = 0;
		int nMarker = 0;
		int nll = 0;
		int nlh = 0;
		int nGot = 0;
		BYTE* pData = NULL;

		if( m_nSectionsRead >= EXIF_MAX_SECTIONS )
		{
			strcpy( m_szLastError, "Too many sections in jpeg file." );
			return false;
		}

		for( a = 0; a < 7; a++ )
		{
			nMarker = p_hFile->GetC();
			if( nMarker != 0xff )
				break;

			if( a >= 6 )
			{
				strcpy( m_szLastError, "Too many padding bytes." );
				return false;
			}
		}
		if( nMarker == 0xff )
		{
			strcpy( m_szLastError, "Too many padding bytes." );
			return false;
		}
		m_vecSections[m_nSectionsRead].Type = nMarker;

		// 读取section长度
		nlh = p_hFile->GetC();
		nll = p_hFile->GetC();
		nItemLen = (nlh << 8) | nll;

		if( nItemLen < 2 )
		{
			strcpy( m_szLastError, "Invalid marker." );
			return false;
		}
		m_vecSections[m_nSectionsRead].Size = nItemLen;

		pData = ( BYTE* )malloc( nItemLen );
		if( pData == NULL )
		{
			strcpy( m_szLastError, "Cann't allocate memory." );
			return false;
		}
		m_vecSections[m_nSectionsRead].Data = pData;

		pData[0] = (BYTE)nlh;
		pData[1] = (BYTE)nll;
		nGot = p_hFile->Read( pData + 2, 1, nItemLen - 2 );	// 读取一个section
		if( nGot != nItemLen - 2)
		{
			strcpy( m_szLastError, "Load section failed." );
			return false;
		}
		m_nSectionsRead += 1;

		// 下面是处理
		switch ( nMarker )
		{
		case M_SOS:
			{
				if( p_nReadMode & eExifReadImage )
				{
					int nCp = 0;
					int nEp = 0;
					int nSize = 0;
					nCp = p_hFile->Tell();
					p_hFile->Seek( 0, SEEK_END );
					nEp = p_hFile->Tell();
					p_hFile->Seek( nCp, SEEK_SET );
					nSize = nEp - nCp;
					pData = ( BYTE* )malloc( nSize );
					if( pData == NULL )
					{
						strcpy( m_szLastError, "Cann't allocate image memory." );
						return false;
					}
					nGot = p_hFile->Read( pData, 1, nSize );
					if( nGot != nSize )
					{
						strcpy( m_szLastError, "Cann't read the rest of image." );
						return false;
					}

					m_vecSections[m_nSectionsRead].Data = pData;
					m_vecSections[m_nSectionsRead].Size = nSize;
					m_vecSections[m_nSectionsRead].Type = PSEUDO_IMAGE_MARKER;
					m_nSectionsRead++;
				}
				return true;
			}
			break;
		case M_EOI:
			{
				return false;
			}
			break;
		case M_COM:
			{
				if( bIsHaveCom || ( p_nReadMode & eExifReadExif ) == 0 )
				{
					free( m_vecSections[--m_nSectionsRead].Data );
					m_vecSections[m_nSectionsRead].Data = NULL;
				}
				else
				{
					_ProcessCom( pData, nItemLen );
					bIsHaveCom = true;
				}
			}
			break;
		case M_JFIF:
			{
				free( m_vecSections[--m_nSectionsRead].Data );
				m_vecSections[m_nSectionsRead].Data = NULL;
			}
			break;
		case M_EXIF:
			{
				if(( p_nReadMode & eExifReadExif ) && ( memcmp( pData + 2, "Exif", 4) == 0 ))
				{
					m_pExifInfo->IsExif	= _ProcessExif( (BYTE*)pData + 2, nItemLen );
				}
				else
				{
					free( m_vecSections[--m_nSectionsRead].Data );
					m_vecSections[m_nSectionsRead].Data = NULL;
				}
			}
			break;
		case M_SOF0:
		case M_SOF1:
		case M_SOF2:
		case M_SOF3:
		case M_SOF5:
		case M_SOF6:
		case M_SOF7:
		case M_SOF9:
		case M_SOF10:
		case M_SOF11:
		case M_SOF13:
		case M_SOF14:
		case M_SOF15:
			{
				_ProcessSOFn( pData, nMarker );
			}
			break;
		default:
			break;
		}
	}
	return true;
}
//-------------------------------------------------------------------------
bool CFKExifInfo::EncodeExif( IFKFile* p_hFile )
{
	if( _FindSection( M_SOS ) == NULL )
	{
		strcpy( m_szLastError, "Can not read all exif." );
		return false;
	}

	p_hFile->PutC( 0xff );
	p_hFile->PutC( 0xd8 );

	if( m_vecSections[0].Type != M_EXIF && m_vecSections[0].Type != M_JFIF )
	{
		static BYTE JfifHead[18] = {
			0xff, M_JFIF,
			0x00, 0x10, 'J' , 'F' , 'I' , 'F' , 0x00, 0x01, 
			0x01, 0x01, 0x01, 0x2C, 0x01, 0x2C, 0x00, 0x00 
		};
		p_hFile->Write( JfifHead, 18, 1 );
	}
	int a = 0;
	for( a = 0; a < m_nSectionsRead - 1; a++ )
	{
		p_hFile->PutC( 0xff );
		p_hFile->PutC( (unsigned char)(m_vecSections[a].Type) );
		p_hFile->Write( m_vecSections[a].Data, m_vecSections[a].Size, 1 );
	}

	p_hFile->Write( m_vecSections[a].Data, m_vecSections[a].Size, 1 );
	return true;
}
//-------------------------------------------------------------------------
void CFKExifInfo::DiscardAllButExif()
{
	SSection tagExifKeeper;
	SSection tagCommonKeeper;
	memset( &tagExifKeeper, 0, sizeof(SSection) );
	memset( &tagCommonKeeper, 0, sizeof(SSection) );

	for( int a = 0; a < m_nSectionsRead; ++a )
	{
		if( m_vecSections[a].Type == M_EXIF && tagExifKeeper.Type == 0 )
		{
			tagExifKeeper = m_vecSections[a];
		}
		else if( m_vecSections[a].Type == M_COM && tagCommonKeeper.Type == 0 )
		{
			tagCommonKeeper = m_vecSections[a];
		}
		else
		{
			free( m_vecSections[a].Data );
			m_vecSections[a].Data = NULL;
		}
	}
	m_nSectionsRead = 0;
	if( tagExifKeeper.Type )
	{
		m_vecSections[m_nSectionsRead++]	= tagExifKeeper;
	}
	if( tagCommonKeeper.Type )
	{
		m_vecSections[m_nSectionsRead++]	= tagCommonKeeper;
	}
}
//-------------------------------------------------------------------------
bool CFKExifInfo::_ProcessExif( unsigned char* p_ucBuf, unsigned int p_unLen )
{
	m_pExifInfo->FlashUsed		= 0;
	m_pExifInfo->Comments[0]	= '\0';
	m_nExifImageWidth			= 0;

	// 检查Exif头是否合法
	static const unsigned char ucExifHeader[] = "Exif\0\0";
	if( memcmp( p_ucBuf + 0, ucExifHeader, 6 ) )
	{
		strcpy( m_szLastError, "Incorrect exif header." );
		return false;
	}

	if( memcmp( p_ucBuf + 6, "II", 2 ) == 0 )
	{
		m_nMotorolaOrder = 0;
	}
	else
	{
		if( memcmp( p_ucBuf + 6, "MM", 2 ) == 0 )
		{
			m_nMotorolaOrder = 1;
		}
		else
		{
			strcpy( m_szLastError, "Incorrect exif marker." );
			return false;
		}
	}

	// 检查下两个值是否合法
	if( _Get16u( p_ucBuf + 8 ) != 0x2a )
	{
		strcpy( m_szLastError, "Incorrect exif start." );
		return false;
	}

	int nFirstOffset = _Get32u( p_ucBuf + 10 );
	unsigned char* pLastExifRef = p_ucBuf;

	if( !_ProcessExifDir( p_ucBuf+14, p_ucBuf+6, p_unLen-6, m_pExifInfo, &pLastExifRef ) )
		return false;

	if( nFirstOffset > 8 )
	{
		if( !_ProcessExifDir( p_ucBuf+14+nFirstOffset-8, p_ucBuf+6, p_unLen-6, m_pExifInfo, &pLastExifRef ) )
			return false;
	}

	if( m_pExifInfo->FocalplaneXRes != 0 )
	{
		m_pExifInfo->CCDWidth = (float)( m_nExifImageWidth * m_pExifInfo->FocalplaneUnits / m_pExifInfo->FocalplaneXRes );
	}
	return true;
}
//-------------------------------------------------------------------------
void CFKExifInfo::_ProcessCom( const BYTE* p_byData, int p_nLen )
{
	int ch = 0;
	char cComment[EXIF_MAX_COMMENT + 1];
	int nCh = 0;
	if( p_nLen > EXIF_MAX_COMMENT )
		p_nLen = EXIF_MAX_COMMENT;

	for( int a = 0; a < p_nLen; ++a )
	{
		ch = p_byData[a];
		if( ch == '\r' && p_byData[a + 1] == '\n' )
			continue;
		if( isprint(ch) || ch == '\n' || ch == '\t' )
		{
			cComment[nCh++] = (char)ch;
		}
		else
		{
			cComment[nCh++] = '?';
		}
	}

	cComment[nCh] = '\0';
	strcpy( m_pExifInfo->Comments, cComment );
}
//-------------------------------------------------------------------------
void CFKExifInfo::_ProcessSOFn( const BYTE* p_byData, int p_nMarker )
{
	int nDataPrecision = 0;
	int nNumComponents = 0;
	nDataPrecision = p_byData[2];
	m_pExifInfo->Height	= _Get16m( (void*)(p_byData + 3) );
	m_pExifInfo->Width = _Get16m( (void*)(p_byData + 5) );
	nNumComponents = p_byData[7];

	if( nNumComponents == 3 )
	{
		m_pExifInfo->IsColor = 1;
	}
	else
	{
		m_pExifInfo->IsColor = 0;
	}
	m_pExifInfo->Process = p_nMarker;
}
//-------------------------------------------------------------------------
bool CFKExifInfo::_ProcessExifDir( unsigned char* p_szDirStart, unsigned char* p_szOffsetBase, unsigned int p_unExifLength,
								SExifInfo* const p_pInfo, unsigned char** const p_ppLastExifRef, int p_nNestingLevel )
{
	int de = 0;
	int nNumDirEntries = 0;
	unsigned int unThumbnailOffset = 0;
	unsigned int unThumbnailSize = 0;

	if( p_nNestingLevel > 4 )
	{
		strcpy( m_szLastError,"Maximum directory nesting exceeded (corrupt exif header)" );
		return false;
	}
	nNumDirEntries = _Get16u( p_szDirStart );
	if(( p_szDirStart + nNumDirEntries * 12 ) > ( p_szOffsetBase + p_unExifLength ))
	{
		strcpy( m_szLastError,"Illegally sized directory" );
		return false;
	}

	for( de = 0; de < nNumDirEntries; de++ )
	{
		int nTag = 0;
		int nFormat = 0;
		int nComponents = 0;
		unsigned char* szValuePtr = NULL;
		int nByteCount = 0;
		unsigned char* szDirEntry = NULL;

		szDirEntry = p_szDirStart + 2 + 12 * de;
		nTag = _Get16u( szDirEntry );
		nFormat = _Get16u( szDirEntry + 2 );
		nComponents = _Get32u( szDirEntry + 4 );

		if( nFormat - 1 >= NUM_FORMATS )
		{
			strcpy( m_szLastError,"Illegal format code in EXIF dir" );
			return false;
		}

		nByteCount = nComponents * s_BytesPerFormat[nFormat];
		if( nByteCount < 4 )
		{
			unsigned int unOffsetVal = 0;
			unOffsetVal = _Get32u( szDirEntry + 8 );
			if( unOffsetVal + nByteCount > p_unExifLength )
			{
				strcpy( m_szLastError,"Illegal pointer offset value in EXIF." );
				return false;
			}
			szValuePtr = p_szOffsetBase + unOffsetVal;
		}
		else
		{
			szValuePtr = szDirEntry + 8;
		}

		if( *p_ppLastExifRef < szValuePtr + nByteCount )
		{
			*p_ppLastExifRef = szValuePtr + nByteCount;
		}

		switch (nTag)
		{
		case TAG_MAKE:
			strncpy( m_pExifInfo->CameraMake, (char*)szValuePtr, 31 );
			break;
		case TAG_MODEL:
			strncpy( m_pExifInfo->CameraModel, (char*)szValuePtr, 39 );
			break;
		case TAG_EXIF_VERSION:
			strncpy( m_pExifInfo->Version, (char*)szValuePtr, 4 );
			break;
		case TAG_DATETIME_ORIGINAL:
			strncpy( m_pExifInfo->DateTime, (char*)szValuePtr, 19 );
			break;
		case TAG_USERCOMMENT:
			{
				for( int a = nByteCount; ;  )
				{
					a--;
					if( ( (char*)szValuePtr )[a] == ' ' )
					{
						((char*)szValuePtr)[a] = '\0';
					}
					else
					{
						break;
					}
					if( a == 0 )
						break;
				}
			}
			break;
		case TAG_FNUMBER:
			m_pExifInfo->ApertureFNumber = ( float )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_APERTURE:
		case TAG_MAXAPERTURE:
			if( m_pExifInfo->ApertureFNumber == 0 )
			{
				m_pExifInfo->ApertureFNumber = ( float )exp(_ConvertAnyFormat(szValuePtr, nFormat) * log(2.0f) * 0.5f );
			}
			break;
		case TAG_BRIGHTNESS:
			m_pExifInfo->Brightness = (float)_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_FOCALLENGTH:
			m_pExifInfo->FocalLength = (float)_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_SUBJECT_DISTANCE:
			m_pExifInfo->Distance = (float)_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_EXPOSURETIME:
			m_pExifInfo->ExposureTime = (float)_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_SHUTTERSPEED:
			if( m_pExifInfo->ExposureTime == 0 )
			{
				m_pExifInfo->ExposureTime = ( float )( 1 / exp( _ConvertAnyFormat(szValuePtr, nFormat) * log(2.0f)));
			}
			break;
		case TAG_FLASH:
			if( (int)_ConvertAnyFormat(szValuePtr, nFormat) & 7 )
			{
				m_pExifInfo->FlashUsed = 1;
			}
			else
			{
				m_pExifInfo->FlashUsed = 0;
			}
			break;
		case TAG_ORIENTATION:
			m_pExifInfo->Orientation	= (int)_ConvertAnyFormat( szValuePtr, nFormat );
			if( m_pExifInfo->Orientation < 1 || m_pExifInfo->Orientation > 8 )
			{
				strcpy( m_szLastError,"Undefined rotation value" );
				m_pExifInfo->Orientation = 0;
			}
			break;
		case TAG_EXIF_IMAGELENGTH:
		case TAG_EXIF_IMAGEWIDTH:
			{
				int n = 0;
				n = ( int )_ConvertAnyFormat( szValuePtr, nFormat );
				if( m_nExifImageWidth < n )
					m_nExifImageWidth = n;
			}
			break;
		case TAG_FOCALPLANEXRES:
			m_pExifInfo->FocalplaneXRes	= ( float )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_FOCALPLANEYRES:
			m_pExifInfo->FocalplaneYRes	= ( float )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_RESOLUTIONUNIT:
			{
				switch ( (int)_ConvertAnyFormat( szValuePtr, nFormat ) )
				{
				case 1: m_pExifInfo->ResolutionUnit	= 1.0f;				break; // 英尺
				case 2: m_pExifInfo->ResolutionUnit = 1.0f;				break;
				case 3: m_pExifInfo->ResolutionUnit = 0.3937007874f;	break; // 厘米
				case 4: m_pExifInfo->ResolutionUnit = 0.03937007874f;	break; // 毫米
				case 5: m_pExifInfo->ResolutionUnit = 0.00003937007874f;break; // 微米
				default:
					break;
				}
			}
			break;
		case TAG_FOCALPLANEUNITS:
			{
				switch ( (int)_ConvertAnyFormat( szValuePtr, nFormat ) )
				{
				case 1: m_pExifInfo->FocalplaneUnits	= 1.0f;				break; // 英尺
				case 2: m_pExifInfo->FocalplaneUnits	= 1.0f;				break;
				case 3: m_pExifInfo->FocalplaneUnits	= 0.3937007874f;	break; // 厘米
				case 4: m_pExifInfo->FocalplaneUnits	= 0.03937007874f;	break; // 毫米
				case 5: m_pExifInfo->FocalplaneUnits	= 0.00003937007874f;break; // 微米
				default:
					break;
				}
			}
			break;
		case TAG_EXPOSURE_BIAS:
			m_pExifInfo->ExposureBias = ( float )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_WHITEBALANCE:
			m_pExifInfo->Whitebalance = ( int )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_METERING_MODE:
			m_pExifInfo->MeteringMode = ( int )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_EXPOSURE_PROGRAM:
			m_pExifInfo->ExposureProgram = ( int )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_ISO_EQUIVALENT:
			m_pExifInfo->ISOequivalent = ( int )_ConvertAnyFormat( szValuePtr, nFormat );
			if( m_pExifInfo->ISOequivalent < 50 )
				m_pExifInfo->ISOequivalent *= 200;
			break;
		case TAG_COMPRESSION_LEVEL:
			m_pExifInfo->CompressionLevel = ( int )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_XRESOLUTION:
			m_pExifInfo->Xresolution = ( float )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_YRESOLUTION:
			m_pExifInfo->Yresolution = ( float )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_THUMBNAIL_OFFSET:
			unThumbnailOffset = ( unsigned int )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		case TAG_THUMBNAIL_LENGTH:
			unThumbnailSize = ( unsigned int )_ConvertAnyFormat( szValuePtr, nFormat );
			break;
		default:
			break;
		}

		if( nTag == TAG_EXIF_OFFSET || nTag == TAG_INTEROP_OFFSET )
		{
			unsigned char* pszSubDirStart = NULL;
			unsigned int unOffset = _Get32u( szValuePtr );
			if( unOffset > 8 )
			{
				pszSubDirStart = p_szOffsetBase + unOffset;
				if( pszSubDirStart < p_szOffsetBase || pszSubDirStart > p_szOffsetBase + p_unExifLength )
				{
					strcpy( m_szLastError,"Illegal subdirectory link" );
					return false;
				}
				_ProcessExifDir( pszSubDirStart, p_szOffsetBase, p_unExifLength, m_pExifInfo, p_ppLastExifRef, p_nNestingLevel + 1 );
			}
			continue;
		}
	}

	{
		unsigned char* szSubDirStart = NULL;
		unsigned int unOffset = 0;
		unOffset = _Get16u( p_szDirStart + 2 + 12 * nNumDirEntries );
		if( unOffset )
		{
			szSubDirStart = p_szOffsetBase + unOffset;
			if( szSubDirStart < p_szOffsetBase || szSubDirStart > p_szOffsetBase + p_unExifLength )
			{
				strcpy( m_szLastError,"Illegal subdirectory link" );
				return false;
			}
			_ProcessExifDir( szSubDirStart, p_szOffsetBase, p_unExifLength, m_pExifInfo, p_ppLastExifRef, p_nNestingLevel + 1 );
		}
	}

	{
		if( unThumbnailSize && unThumbnailOffset )
		{
			if( unThumbnailSize + unThumbnailOffset <= p_unExifLength )
			{
				m_pExifInfo->ThumbnailPointer = p_szOffsetBase + unThumbnailOffset;
				m_pExifInfo->ThumbnailSize = unThumbnailSize;
			}
		}
	}

	return true;
}
//-------------------------------------------------------------------------
int CFKExifInfo::_Get16u( void* p_pShort )
{
	if( m_nMotorolaOrder )
	{
		return (((unsigned char*)p_pShort)[0] << 8 ) |((unsigned char*)p_pShort)[1];
	}
	else
	{
		return (((unsigned char*)p_pShort)[1] << 8 ) |((unsigned char*)p_pShort)[0];
	}
}
//-------------------------------------------------------------------------
int CFKExifInfo::_Get16m( void* p_pShort )
{
	return (((unsigned char*)p_pShort)[0] << 8 ) | ((unsigned char*)p_pShort)[1];
}
//-------------------------------------------------------------------------
long CFKExifInfo::_Get32s( void* p_pLong )
{
	if( m_nMotorolaOrder )
	{
		return (((char*)p_pLong)[0] << 24) | (((char*)p_pLong)[1] << 16) |
			(((char*)p_pLong)[2] << 8) | (((char*)p_pLong)[3] << 0);
	}
	else
	{
		return (((char*)p_pLong)[3] << 24) | (((char*)p_pLong)[2] << 16) |
			(((char*)p_pLong)[1] << 8) | (((char*)p_pLong)[0] << 0);
	}
}
//-------------------------------------------------------------------------
unsigned long CFKExifInfo::_Get32u( void* p_pLong )
{
	return (unsigned long)_Get32s( p_pLong ) & 0xffffffff;
}
//-------------------------------------------------------------------------
double CFKExifInfo::_ConvertAnyFormat( void* p_pValuePtr, int p_nFormat )
{
	double dValue = 0.0;
	switch( p_nFormat )
	{
	case FMT_SBYTE:
		dValue = *(signed char*)p_pValuePtr;
		break;
	case FMT_BYTE:
		dValue = *(unsigned char*)p_pValuePtr;
		break;
	case FMT_USHORT:
		dValue = _Get16u(p_pValuePtr);
		break;
	case FMT_ULONG:
		dValue = _Get32u(p_pValuePtr);
		break;
	case FMT_URATIONAL:
	case FMT_SRATIONAL:
		{
			int nNum = 0;
			int nDen = 0;
			nNum = _Get32s( p_pValuePtr );
			nDen = _Get32s( 4 + (char*)p_pValuePtr );
			if( nDen == 0 )
				dValue = 0;
			else
				dValue = (double)(nNum / nDen);
		}
		break;
	case FMT_SSHORT:
		dValue = (signed short)(_Get16u(p_pValuePtr));
		break;
	case FMT_SLONG:
		dValue = _Get32s(p_pValuePtr);
		break;
	case FMT_SINGLE:
		dValue = (double)*(float*)(p_pValuePtr);
		break;
	case FMT_DOUBLE:
		dValue = *(double*)p_pValuePtr;
		break;
	}
	return dValue;
}
//-------------------------------------------------------------------------
void* CFKExifInfo::_FindSection( int p_nSectionType )
{
	int a = 0;
	for( a = 0; a < m_nSectionsRead - 1; a++ )
	{
		if( m_vecSections[a].Type == p_nSectionType )
		{
			return &m_vecSections[a];
		}
	}
	return NULL;
}
//-------------------------------------------------------------------------
#endif
//-------------------------------------------------------------------------
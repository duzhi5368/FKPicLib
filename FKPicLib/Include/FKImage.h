//*************************************************************************
//	��������:	2014-9-11   16:57
//	�ļ�����:	FKImage.h
//  �� �� ��:   ������ FreeKnight	
//	��Ȩ����:	MIT
//	˵    ��:	
//*************************************************************************

#pragma once

//-------------------------------------------------------------------------
#include "FKPicCommonDef.h"
//-------------------------------------------------------------------------
class IFKFile;
class CFKImage
{
	typedef struct SFKImageInfo
	{
		DWORD			m_dwEffWidth;				// ɨ���߿��
		BYTE*			m_byImage;					// ͼ��ʵ������
		CFKImage*		m_pGhost;					// ������ͼ��һ�����ݣ����ֵָ����ԭʼ����
		CFKImage*		m_pParent;					// ������ͼ��һ���㣬���ֵָ��������
		DWORD			m_dwType;					// ԭʼͼ�θ�ʽ
		char			m_szLastError[256];			// ������Ϣ�����Ĵ�����Ϣ
		long			m_lProgress;				// ����ѭ���Ĵ���
		long			m_lEscape;					// ������־
		long			m_lBkgndIndex;				// ����ɫ������GIF,PNG,MNGʹ�ã�
		SRGBQuad		m_tagBkgndColor;			// ����ɫ��RGB��ԭɫ��
		float			m_fQuality;					// JPEGƷ�ʣ�0.0f - 100.0f, JEPGʹ�� ��
		BYTE			m_byJpegScale;				// JPEG���űȣ�JPEGʹ�ã�
		long			m_lFrame;					// ��ǰ֡����TIF��GIF��MNGʹ�ã�
		long			m_lNumFrames;				// ��֡����TIF��GIF��MNGʹ�ã�
		DWORD			m_dwFrameDelay;				// ֡�����GIF��MNGʹ�ã�
		long			m_lXDpi;					// ˮƽ�ֱ���
		long			m_lYDpi;					// ��ֱ�ֱ���
		SRect			m_tagSelectionBox;			// ��Χ��
		BYTE			m_byAlphaMax;				// ���͸����
		bool			m_bAlphaPaletteEnable;		// �����ɫ������Alphaͨ�������ֵΪtrue
		bool			m_bEnabled;					// �Ƿ�����ͼ����
		long			m_lXOffset;					// Xƫ����
		long			m_lYOffset;					// Yƫ����
		DWORD			m_dwCodeOpt[eFormats_Max];	// һЩ����ѡ�GIF,TIFʹ�ã�
		SRGBQuad		m_tagLastC;					// һЩ�Ż�ѡ��
		BYTE			m_byLastCIndex;
		bool			m_bLastCIsValid;
		long			m_lNumLayers;				// �ܲ���
		DWORD			m_dwFlag;					// ��ʾλ��0x??000000 ����λ, 0x00??0000 ���ģʽ, 0x0000???? ��ID��
		BYTE			m_byDispmeth;
		bool			m_bGetAllFrames;
		bool			m_bLittleEndianHost;		// �Ƿ�Сβ����
		BYTE			m_byZipScale;				// Zipѹ������(FKPʹ��)
	}FKIMAGEINFO;

public:
	CFKImage( DWORD p_dwImageType = 0 );
	virtual ~CFKImage();
	CFKImage& operator = ( const CFKImage& p_tagOther );
public:
	void*				Create( DWORD p_dwWidth, DWORD p_dwHeight, DWORD p_dwBpp, DWORD p_dwImageType = 0 );
	bool				Destroy();
	bool				DestroyFrames();
	void				Copy( const CFKImage& p_tagSrc, bool p_bCopyPixels = true, 
		bool p_bCopySelection = true, bool p_bCopyAlpha = true );
	bool				Load( const TCHAR* p_szFileName, DWORD p_dwImageType = 0 );
	bool				Decode( FILE* p_pFile, DWORD p_dwImageType );
	bool				Decode( IFKFile* p_pFile, DWORD p_dwImageType );
	bool				Decode( BYTE* p_szBuffer, DWORD p_dwSize, DWORD p_dwImageType );
	bool				Transfer( CFKImage &p_From, bool p_bTransferFrames = true );
	bool				Save( const TCHAR* p_szFileName, DWORD p_dwImageType = 0 );
	bool				Encode( FILE* p_pFile, DWORD p_dwImageType );
	bool				Encode( IFKFile* p_pFile, DWORD p_dwImageType );
	bool				Encode( BYTE*& p_Buffer, long& p_dwSize, DWORD p_dwImageType );
public:
	long				GetXDpi() const;
	long				GetYDpi() const;
	void				SetXDpi( long p_lDpi );
	void				SetYDpi( long p_lDpi );
	DWORD				GetClrImportant() const;
	void				SetClrImportant( DWORD p_dwColors = 0 );
	SRGBQuad			GetTransColor();
	long				GetSize();
	DWORD				GetPaletteSize();
	SRGBQuad*			GetPalette() const;
	void				SetGrayPalette();
	void				SetPalette( DWORD p_dwN, BYTE* p_byRed, BYTE* p_byGreen, BYTE* p_byBlue );
	void				SetPalette( SRGBQuad* p_pPalette,DWORD p_dwColors = 256 );
	void				SetPalette( SRGBColor* p_pRgb,DWORD p_dwColors = 256 );
	SRGBQuad			GetPaletteColor( BYTE p_byIndex );
	bool				GetPaletteColor( BYTE p_byIndex, BYTE* p_pRed, BYTE* p_pGreen, BYTE* p_pBlue );
	void				SetPaletteColor( BYTE p_byIndx, SRGBQuad p_tagRGB );
	void				SetPaletteColor( BYTE p_byIndx, BYTE p_byRed, BYTE p_byGreen, BYTE p_byBlue, BYTE p_byAlpha = 0 );
	void				SwapIndex( BYTE p_byIndex1, BYTE p_byIndex2 );
	BYTE*				GetBits( DWORD p_dwRow = 0 );
	DWORD				GetHeight() const;
	DWORD				GetWidth() const;
	DWORD				GetNumColors() const;
	WORD				GetBpp() const;
	DWORD				GetType() const;
	DWORD				GetEffWidth() const;
	BYTE				GetJpegQuality() const;
	void				SetJpegQuality( BYTE p_byQuality );
	BYTE				GetJpegScale() const;
	void				SetJpegScale( BYTE p_byScale );
	void				SetZipQuality( BYTE p_byQuality );
	BYTE				GetZipQuality() const;
	bool				IsGrayScale();
	DWORD				GetCodecOption( DWORD p_dwImageType = 0 );
	bool				SetCodecOption( DWORD p_dwOpt, DWORD p_dwImageType = 0 );
	const char*			GetLastError();
	bool				IsInside( long p_lX, long p_lY );
	bool				Flip( bool p_bFlipSelection = false, bool p_bFlipAlpha = true );
	SRGBQuad			GetPixelColor( long p_lX, long p_lY, bool p_bGetAlpha = true );


#if USE_FK_SELECTION
	bool				DeleteSelection();
	bool				FlipSelection();
#endif

#if USE_FK_ALPHA
	void				DeleteAlpha();
	bool				CreateAlpha();
	bool				IsAlphaValid();
	void				SetAlpha( const long p_lX, const long p_lY, const BYTE p_byLevel );
	BYTE				GetAlpha( const long p_lX, const long p_lY );
	void				InvertAlpha();
	bool				FilpAlpha();
	BYTE*				GetAlphaPointer( const long p_lX = 0, const long p_lY = 0 );
protected:
	BYTE				_BlindGetAlpha( const long p_lX, const long p_lY );
#endif

protected:
	void				_StartUp( DWORD p_dwImageType );
	bool				_IsLittleEndian();
	void				_CopyInfo( const CFKImage& p_Src );
	void				_Ghost( const CFKImage* p_pSrc );
	long				_Ntohl( const long p_lDword );
	short				_Ntohs( const short p_sWord );
	void				_RGBtoBGR( BYTE* p_pBuffer, int p_nLength );
	void				_Bitfield2RGB( BYTE* p_pSrc, DWORD p_dwRedMask, 
		DWORD p_dwGreenMask, DWORD p_dwBlueMask, BYTE p_byBpp);
	void				_Bihtoh( SBitmapInfoHeader* p_pBih );
	bool				_EncodeSafeCheck( IFKFile* p_hFile );
	BYTE				_BlindGetPixelIndex( const long x, const long y );
	void				_BlindSetPixelIndex( long x, long y, BYTE p_byByte );
	SRGBQuad			_BlindGetPixelColor( long p_lX, long p_lY, bool p_bGetAlpha = true );
	bool				_IsHadFreeKnightSign( IFKFile* p_hFile );
protected:
	void*				m_pDib;						// �����ļ�ͷ����ɫ�壬��������
	SBitmapInfoHeader	m_tagHead;					// ��׼�ļ�ͷ
	SFKImageInfo		m_tagInfo;					// ������Ϣ
	BYTE*				m_pSelection;				// ѡ������
	BYTE*				m_pAlpha;					// Alphaͨ��
	CFKImage**			m_ppLayers;					// �Ӳ㼶
	CFKImage**			m_ppFrames;					// ��֡
};
//-------------------------------------------------------------------------
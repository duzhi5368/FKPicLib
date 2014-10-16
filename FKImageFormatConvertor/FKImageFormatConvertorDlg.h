
// FKImageFormatConvertorDlg.h : ͷ�ļ�
//

#pragma once


// CFKImageFormatConvertorDlg �Ի���
class CFKImage;
class CFKImageFormatConvertorDlg : public CDialogEx
{
// ����
public:
	CFKImageFormatConvertorDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_FKIMAGEFORMATCONVERTOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
public:
	CFKImage*		m_pImage;
	char			m_szSrcFilePath[MAX_PATH];
	char			m_szSrcDirPath[MAX_PATH];
	char			m_szDstDirPath[MAX_PATH];
protected:
	// ������ȵݹ����Ŀ¼�����е��ļ�
	bool			DirectoryList(LPCSTR Path, int p_nSrcType, int p_nDstType, int& p_nFileCount, int& p_nFileError );
	int				GetZipRate();
	void			SetZipRate( int p_nValue );
	int				GetJpegRate();
	void			SetJpegRate( int p_nValue );
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
};


// FKImageFormatConvertorDlg.h : 头文件
//

#pragma once


// CFKImageFormatConvertorDlg 对话框
class CFKImage;
class CFKImageFormatConvertorDlg : public CDialogEx
{
// 构造
public:
	CFKImageFormatConvertorDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_FKIMAGEFORMATCONVERTOR_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:
	CFKImage*		m_pImage;
	char			m_szSrcFilePath[MAX_PATH];
	char			m_szSrcDirPath[MAX_PATH];
	char			m_szDstDirPath[MAX_PATH];
protected:
	// 深度优先递归遍历目录中所有的文件
	bool			DirectoryList(LPCSTR Path, int p_nSrcType, int p_nDstType, int& p_nFileCount, int& p_nFileError );
	int				GetZipRate();
	void			SetZipRate( int p_nValue );
	int				GetJpegRate();
	void			SetJpegRate( int p_nValue );
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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

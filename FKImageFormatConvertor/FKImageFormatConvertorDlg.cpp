
// FKImageFormatConvertorDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FKImageFormatConvertor.h"
#include "FKImageFormatConvertorDlg.h"
#include "afxdialogex.h"
#include "../FKPicLib/Include/FKImage.h"
#include <string>
using std::string;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CFKImageFormatConvertorDlg 对话框



CFKImageFormatConvertorDlg::CFKImageFormatConvertorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFKImageFormatConvertorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFKImageFormatConvertorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFKImageFormatConvertorDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CFKImageFormatConvertorDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CFKImageFormatConvertorDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CFKImageFormatConvertorDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CFKImageFormatConvertorDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CFKImageFormatConvertorDlg::OnBnClickedButton5)
END_MESSAGE_MAP()


// CFKImageFormatConvertorDlg 消息处理程序

int CFKImageFormatConvertorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	m_pImage = NULL;

	memset( m_szDstDirPath, 0, MAX_PATH );
	memset( m_szSrcDirPath, 0, MAX_PATH );
	memset( m_szSrcFilePath, 0, MAX_PATH );

	CSliderCtrl* p = (CSliderCtrl*)GetDlgItem(IDC_SLIDER1);
	if( p )
	{
		p->SetRange(1, 9);
	}
	p = (CSliderCtrl*)GetDlgItem(IDC_SLIDER2);
	if( p )
	{
		p->SetRange(50, 100);
	}

	CComboBox* pComboBox = (CComboBox*)GetDlgItem( IDC_COMBO2 );
	if( pComboBox )
	{
		pComboBox->AddString("BMP");
		pComboBox->AddString("JPG");
		pComboBox->AddString("PNG");
		pComboBox->AddString("FKP");
		pComboBox->SetCurSel( 3 );
	}
	pComboBox = (CComboBox*)GetDlgItem( IDC_COMBO3 );
	if( pComboBox )
	{
		pComboBox->AddString("BMP");
		pComboBox->AddString("JPG");
		pComboBox->AddString("PNG");
		pComboBox->AddString("FKP");
		pComboBox->SetCurSel( 2 );
	}
	pComboBox = (CComboBox*)GetDlgItem( IDC_COMBO4 );
	if( pComboBox )
	{
		pComboBox->AddString("BMP");
		pComboBox->AddString("JPG");
		pComboBox->AddString("PNG");
		pComboBox->AddString("FKP");
		pComboBox->SetCurSel( 3 );
	}

	SetZipRate( 9 );
	SetJpegRate( 75 );

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFKImageFormatConvertorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CFKImageFormatConvertorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
//-------------------------------------------------------------------------
/// 分解文件路径字串
static bool ParseFileName( const std::string& strFilePathName, std::string& strPathName, std::string& strFileName, std::string& strFileExtName )
{
	if( strFilePathName.empty() )
	{
		return false;
	}

	strPathName.clear();
	strFileName.clear();
	strFileExtName.clear();

	std::size_t point_pos = strFilePathName.rfind( '.' ); 

	std::size_t pos = strFilePathName.rfind('\\');
	if( pos == std::string::npos )
	{
		pos = strFilePathName.rfind('/');
		if( pos == std::string::npos )
		{
			if( point_pos == std::string::npos )
			{
				return false;
			}
		}
	}

	if( point_pos == std::string::npos )
	{
		strPathName = strFilePathName;
	}
	else
	{
		/// 以斜杠结尾
		if( pos == strFilePathName.length() - 1 )
		{
			strPathName = strFilePathName.substr( 0, pos );
		}
		else
		{
			if( pos != std::string::npos )
			{
				strPathName = strFilePathName.substr( 0, pos );
			}

			strFileName = strFilePathName.substr( pos+1, point_pos - pos - 1 );
			strFileExtName = strFilePathName.substr( point_pos + 1, strFilePathName.length() - point_pos - 1 );
		}
	}

	return true;
}
//-------------------------------------------------------------------------
// 深度优先递归遍历目录中所有的文件
bool CFKImageFormatConvertorDlg::DirectoryList(LPCSTR Path, int p_nSrcType, int p_nDstType, int& p_nFileCount, int& p_nFileError )
{
	if( m_pImage )
	{
		delete m_pImage;
		m_pImage = NULL;
	}
	m_pImage = new CFKImage;
	m_pImage->SetJpegQuality( GetJpegRate() );
	m_pImage->SetZipQuality( GetZipRate() );

	string szNewExt = "";
	switch (p_nDstType)
	{
	case eFormats_BMP:
		szNewExt = ".bmp";
		break;
	case eFormats_JPG:
		szNewExt = ".jpg";
		break;
	case eFormats_PNG:
		szNewExt = ".png";
		break;
	case eFormats_FKP:
		szNewExt = ".fkp";
		break;
	default:
		return false;
		break;
	}

	WIN32_FIND_DATA FindData;
	HANDLE hError;
	char FilePathName[1024];
	// 构造路径
	char FullPathName[1024];
	strcpy(FilePathName, Path);
	strcat(FilePathName, "\\*.*");
	hError = FindFirstFile(FilePathName, &FindData);
	if (hError == INVALID_HANDLE_VALUE)
	{
		char szInfo[MAX_PATH];
		sprintf( szInfo, "搜索文件夹失败" );
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
		return false;
	}
	while(::FindNextFile(hError, &FindData))
	{
		// 过虑.和..
		if (strcmp(FindData.cFileName, ".") == 0 
			|| strcmp(FindData.cFileName, "..") == 0 )
		{
			continue;
		}

		// 构造完整路径
		wsprintf(FullPathName, "%s\\%s", Path,FindData.cFileName);
		string szFilePath = string(FullPathName);
		string srcPath;
		string srcFile;
		string srcExt;
		ParseFileName( szFilePath, srcPath, srcFile, srcExt );

		switch( p_nSrcType )
		{
		case eFormats_PNG:
			if( strcmp( srcExt.c_str(), "png" ) == 0 || 
				strcmp( srcExt.c_str(), "PNG" ) == 0)
			{
				if( !m_pImage->Load( szFilePath.c_str(), eFormats_Unknown ))
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "读取文件失败 %s", szFilePath.c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					continue;
				}
				if( !m_pImage->Save( string(srcPath + "\\" + srcFile + szNewExt).c_str(), p_nDstType ) )
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "保存文件失败 %s", string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					p_nFileError++;
					continue;
				}
				else
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "第 %d 文件 转换文件成功 %s", p_nFileCount+1, string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 

					// 删除源文件
					DeleteFile( FullPathName );
					p_nFileCount++;
				}
			}
			break;
		case eFormats_JPG:
			if( strcmp( srcExt.c_str(), "jpg" ) == 0 || 
				strcmp( srcExt.c_str(), "JPG" ) == 0  )
			{
				if( !m_pImage->Load( szFilePath.c_str(), eFormats_Unknown ))
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "读取文件失败 %s", szFilePath.c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					continue;
				}
				if( !m_pImage->Save( string(srcPath + "\\" + srcFile + szNewExt).c_str(), p_nDstType ) )
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "保存文件失败 %s", string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					p_nFileError++;
					continue;
				}
				else
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "第 %d 文件 转换文件成功 %s", p_nFileCount+1, string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 

					// 删除源文件
					DeleteFile( FullPathName );
					p_nFileCount++;
				}
			}
			break;
		case eFormats_BMP:
			if( strcmp( srcExt.c_str(), "bmp" ) == 0 || 
				strcmp( srcExt.c_str(), "BMP" ) == 0)
			{
				if( !m_pImage->Load( szFilePath.c_str(), eFormats_Unknown ))
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "读取文件失败 %s", szFilePath.c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					continue;
				}
				if( !m_pImage->Save( string(srcPath + "\\" + srcFile + szNewExt).c_str(), p_nDstType ) )
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "保存文件失败 %s", string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					p_nFileError++;
					continue;
				}
				else
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "第 %d 文件 转换文件成功 %s", p_nFileCount+1, string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 

					// 删除源文件
					DeleteFile( FullPathName );
					p_nFileCount++;
				}
			}
			break;
		case eFormats_FKP:
			if( strcmp( srcExt.c_str(), "fkp" ) == 0 || 
				strcmp( srcExt.c_str(), "FKP" ) == 0 )
			{
				if( !m_pImage->Load( szFilePath.c_str(), eFormats_Unknown ))
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "读取文件失败 %s", szFilePath.c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					continue;
				}
				if( !m_pImage->Save( string(srcPath + "\\" + srcFile + szNewExt).c_str(), p_nDstType ) )
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "保存文件失败 %s", string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
					p_nFileError++;
					continue;
				}
				else
				{
					char szInfo[MAX_PATH];
					sprintf( szInfo, "第 %d 文件 转换文件成功 %s", p_nFileCount+1, string(srcPath + "\\" + srcFile + szNewExt).c_str() );
					GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 

					// 删除源文件
					DeleteFile( FullPathName );
					p_nFileCount++;
				}
			}
			break;
		default:
			break;
		}

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DirectoryList(FullPathName, p_nSrcType, p_nDstType, p_nFileCount, p_nFileError );
		}
	}
	return true;
}
//-------------------------------------------------------------------------
int CFKImageFormatConvertorDlg::GetZipRate()
{
	CSliderCtrl* p = (CSliderCtrl*)GetDlgItem(IDC_SLIDER1);
	int nPos = 0;
	if( p )
	{
		nPos = p->GetPos();
	}
	return nPos;
}
//-------------------------------------------------------------------------
void CFKImageFormatConvertorDlg::SetZipRate( int p_nValue )
{
	if( p_nValue <= 0 || p_nValue >= 10 )
		return;
	CSliderCtrl* p = (CSliderCtrl*)GetDlgItem(IDC_SLIDER1);
	if( p )
	{
		p->SetPos( p_nValue );
	}
}
//-------------------------------------------------------------------------
int CFKImageFormatConvertorDlg::GetJpegRate()
{
	CSliderCtrl* p = (CSliderCtrl*)GetDlgItem(IDC_SLIDER2);
	int nPos = 0;
	if( p )
	{
		nPos = p->GetPos();
	}
	return nPos;
}
//-------------------------------------------------------------------------
void CFKImageFormatConvertorDlg::SetJpegRate( int p_nValue )
{
	if( p_nValue < 50 || p_nValue > 100 )
		return;
	CSliderCtrl* p = (CSliderCtrl*)GetDlgItem(IDC_SLIDER2);
	if( p )
	{
		p->SetPos( p_nValue );
	}
}
//-------------------------------------------------------------------------
// 打开文件
void CFKImageFormatConvertorDlg::OnBnClickedButton1()
{
	CString szFilePathName;
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY, _T("jpg文件 (*.jpg)|*.jpg|png文件 (*.png)|*.png|bmp文件 (*.bmp)|*.bmp|fkp文件 (*.fkp)|*.fkp|全部文件 (*.*)|*.*||"), NULL);
	if( dlg.DoModal()==IDOK )
	{
		szFilePathName=dlg.GetPathName();
	}
	CEdit* pFileNameEdit = ( CEdit* )( GetDlgItem(IDC_EDIT1) );
	if( pFileNameEdit )
	{
		pFileNameEdit->SetWindowText( szFilePathName.GetBuffer() );
	}

	// 使用FKImage库进行格式转换
	if( m_pImage != NULL )
	{
		delete m_pImage;
		m_pImage = NULL;
	}
	m_pImage = new CFKImage;
	if( !m_pImage->Load( szFilePathName.GetBuffer(), eFormats_Unknown ))
	{
		MessageBox( m_pImage->GetLastError(), "错误", MB_OK );
		return;
	}

	// 保存路径
	strcpy( m_szSrcFilePath, szFilePathName.GetBuffer() );
}
//-------------------------------------------------------------------------
// 开始单文件转换
void CFKImageFormatConvertorDlg::OnBnClickedButton2()
{
	if( m_pImage == NULL )
		return;
	int nDstType = -1;
	CComboBox* pComboBox = (CComboBox*)GetDlgItem( IDC_COMBO2 );
	if( pComboBox )
	{
		nDstType = pComboBox->GetCurSel();
		nDstType += 1;
	}
	if( nDstType < 0 || nDstType >= eFormats_Max )
		return;
	string srcFilePath = string( m_szSrcFilePath );
	string srcPath;
	string srcFile;
	string srcExt;
	ParseFileName( srcFilePath, srcPath, srcFile, srcExt );
	string szDstFilePath;
	switch ( nDstType )
	{
	case eFormats_PNG:
		szDstFilePath = srcPath + "\\" + srcFile + ".png";
		break;
	case eFormats_JPG:
		szDstFilePath = srcPath + "\\" + srcFile + ".jpg";
		break;
	case eFormats_BMP:
		szDstFilePath = srcPath + "\\" + srcFile + ".bmp";
		break;
	case eFormats_FKP:
		szDstFilePath = srcPath + "\\" + srcFile + ".fkp";
		break;
	default:
		break;
	}
	m_pImage->SetJpegQuality( GetJpegRate() );
	m_pImage->SetZipQuality( GetZipRate() );
	if( m_pImage->Save( szDstFilePath.c_str(), nDstType ) )
	{
		char szInfo[MAX_PATH];
		sprintf( szInfo, "转换图片成功: %s", szDstFilePath.c_str() );
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo );
	}
	else
	{
		char szInfo[MAX_PATH];
		sprintf( szInfo, "转换图片失败 %s", m_pImage->GetLastError() );
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo );
	}
}
//-------------------------------------------------------------------------
// 源文件夹打开
void CFKImageFormatConvertorDlg::OnBnClickedButton3()
{
	ZeroMemory(m_szSrcDirPath, sizeof(m_szSrcDirPath));     

	BROWSEINFO bi;     
	bi.hwndOwner = m_hWnd;     
	bi.pidlRoot = NULL;     
	bi.pszDisplayName = m_szSrcDirPath;     
	bi.lpszTitle = "请选择需要转换的源目录：";     
	bi.ulFlags = 0;     
	bi.lpfn = NULL;     
	bi.lParam = 0;     
	bi.iImage = 0;     
	//弹出选择目录对话框  
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);     

	if(lp && SHGetPathFromIDList(lp, m_szSrcDirPath))     
	{  
		char szInfo[MAX_PATH];
		sprintf( szInfo, "选择的源目录为: %s", m_szSrcDirPath );
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
	}  
	else
	{
		AfxMessageBox("无效的目录，请重新选择");
	}

	CEdit* pFileNameEdit = ( CEdit* )( GetDlgItem(IDC_EDIT2) );
	if( pFileNameEdit )
	{
		pFileNameEdit->SetWindowText( m_szSrcDirPath );
	}
}
//-------------------------------------------------------------------------
// 目标文件夹
void CFKImageFormatConvertorDlg::OnBnClickedButton4()
{
	ZeroMemory(m_szDstDirPath, sizeof(m_szDstDirPath));     

	BROWSEINFO bi;     
	bi.hwndOwner = m_hWnd;     
	bi.pidlRoot = NULL;     
	bi.pszDisplayName = m_szDstDirPath;     
	bi.lpszTitle = "请选择需要转换后的导出目录：";     
	bi.ulFlags = 0;     
	bi.lpfn = NULL;     
	bi.lParam = 0;     
	bi.iImage = 0;     
	//弹出选择目录对话框  
	LPITEMIDLIST lp = SHBrowseForFolder(&bi);     

	if(lp && SHGetPathFromIDList(lp, m_szDstDirPath))     
	{  
		char szInfo[MAX_PATH];
		sprintf( szInfo, "选择的目标目录为: %s", m_szDstDirPath );
		GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
	}  
	else
	{
		AfxMessageBox("无效的目录，请重新选择");
	}

	CEdit* pFileNameEdit = ( CEdit* )( GetDlgItem(IDC_EDIT3) );
	if( pFileNameEdit )
	{
		pFileNameEdit->SetWindowText( m_szDstDirPath );
	}
}
//-------------------------------------------------------------------------
// 开始文件夹批量转换
void CFKImageFormatConvertorDlg::OnBnClickedButton5()
{
	char szInfo[MAX_PATH];
	string szSrcDir = string( m_szSrcDirPath );
	string szDstDir = string( m_szDstDirPath );
	if( szSrcDir.empty() || szDstDir.empty() )
		return;

	int nSrcType = -1;
	int nDstType = -1;
	CComboBox* pComboBox = (CComboBox*)GetDlgItem( IDC_COMBO3 );
	if( pComboBox )
	{
		nSrcType = pComboBox->GetCurSel();
		nSrcType += 1;
	}
	pComboBox = (CComboBox*)GetDlgItem( IDC_COMBO4 );
	if( pComboBox )
	{
		nDstType = pComboBox->GetCurSel();
		nDstType += 1;
	}
	if( nDstType < 0 || nDstType >= eFormats_Max ||
		nSrcType < 0 || nSrcType >= eFormats_Max )
		return;

	// step1: copy dir
	char szCopyCmd[MAX_PATH];
	memset( szCopyCmd, 0, MAX_PATH );
	sprintf( szCopyCmd,"xcopy %s %s /e",szSrcDir.c_str(), szDstDir.c_str());
	system( szCopyCmd );
	
	memset( szInfo, 0, MAX_PATH );
	sprintf( szInfo, "拷贝文件夹完成，请等待下一步" );
	GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 

	// step2: convert all
	int nCount = 0;
	int nErrorCount = 0;
	DirectoryList( szDstDir.c_str(), nSrcType, nDstType, nCount, nErrorCount );

	memset( szInfo, 0, MAX_PATH );
	sprintf( szInfo, "格式转换完成 成功 %d 个文件，失败 %d 个文件", nCount, nErrorCount );
	GetDlgItem(IDC_STATIC_INFO)->SetWindowText( szInfo ); 
}
//-------------------------------------------------------------------------

// FKImageFormatConvertor.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CFKImageFormatConvertorApp:
// �йش����ʵ�֣������ FKImageFormatConvertor.cpp
//

class CFKImageFormatConvertorApp : public CWinApp
{
public:
	CFKImageFormatConvertorApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CFKImageFormatConvertorApp theApp;
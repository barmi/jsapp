
// jsapp.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CjsappApp:
// �� Ŭ������ ������ ���ؼ��� jsapp.cpp�� �����Ͻʽÿ�.
//

class CjsappApp : public CWinApp
{
public:
	CjsappApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CjsappApp theApp;
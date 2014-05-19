
// jsappDlg.cpp : ���� ����
//

#include "stdafx.h"
#include "jsapp.h"
#include "jsappDlg.h"
#include "afxdialogex.h"

#include <Vfw.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

// �����Դϴ�.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CjsappDlg ��ȭ ����



CjsappDlg::CjsappDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CjsappDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CjsappDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CjsappDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CjsappDlg �޽��� ó����

BOOL CjsappDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	//ModifyStyleEx(WS_EX_APPWINDOW,WS_EX_TOOLWINDOW);
	//ModifyStyle

#ifdef _DEBUG
	SetTimer(1, 5*1000, NULL);
#else
	SetTimer(1, 5*60*1000, NULL);
#endif

	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

void CjsappDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CjsappDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ�Դϴ�.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CjsappDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CjsappDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	lpwndpos->flags &= ~SWP_SHOWWINDOW;

	CDialogEx::OnWindowPosChanging(lpwndpos);

	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰��մϴ�.
}


void CjsappDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)
	{
		KillTimer(1);
		SaveImageCapture();
#ifdef _DEBUG
		SetTimer(1, 5*1000, NULL);
#else
		SetTimer(1, 5*60*1000, NULL);
#endif
	}

	CDialogEx::OnTimer(nIDEvent);
}


bool CjsappDlg::SaveImageCapture(void)
{
	int nx = 0, ny = 0;
	CImage capImage, capImage2;
	CWnd *pDesktopWnd = CWnd::FromHandle(::GetDesktopWindow());
	HDC hDC = NULL;
	CWindowDC DeskTopDC(pDesktopWnd);
	nx = GetSystemMetrics(SM_CXSCREEN);
	ny = GetSystemMetrics(SM_CYSCREEN);
	if (!capImage.Create(nx, ny, 32))
		return false;
	hDC = capImage.GetDC();
	BitBlt(hDC, 0, 0, nx, ny, DeskTopDC.m_hDC, 0, 0, SRCCOPY);
	//capImage.Save(_T("test.jpg"), Gdiplus::ImageFormatJPEG);
	//capImage.ReleaseDC();

	IStream* pIStream = NULL;
	if (CreateStreamOnHGlobal(NULL, TRUE, (LPSTREAM*)&pIStream) != S_OK)
	{
		return false;
	}
	capImage.Save(pIStream, Gdiplus::ImageFormatJPEG);

	ULARGE_INTEGER ulnSize;
	LARGE_INTEGER lnOffset;
	lnOffset.QuadPart = 0;
	if (pIStream->Seek(lnOffset, STREAM_SEEK_END, &ulnSize) != S_OK)
	{
		pIStream->Release();
		return false;
	}

	if (pIStream->Seek(lnOffset, STREAM_SEEK_SET, NULL) != S_OK)
	{
		pIStream->Release();
		return false;
	}

	char* pBuff = new char[(unsigned int)ulnSize.QuadPart];
	TRACE("++++++++++ulnSize = %ld, pBuff = %p\n", ulnSize.QuadPart, pBuff);
	ULONG ulByteRead;
	if (pIStream->Read(pBuff, (ULONG)ulnSize.QuadPart, &ulByteRead) != S_OK)
	{
		pIStream->Release();
		delete pBuff;
		return false;
	}

	//
	// Send JPEG Memory
	//SendPacket(PACKETTYPE_JPEG_RESPONSE, pBuff, (DWORD)ulnSize.QuadPart);
	/*
	CByteArray buffer;
	buffer.SetSize((INT_PTR)ulnSize.QuadPart + sizeof(DWORD));
	DWORD PacketType = PACKETTYPE_JPEG_RESPONSE;
	memcpy(buffer.GetData(), &PacketType, sizeof(DWORD));
	memcpy(buffer.GetData() + sizeof(DWORD), pBuff, (size_t)ulnSize.QuadPart);
	m_pClientThread->Send(buffer);
	*/
	TCHAR dir[MAX_PATH];
	struct tm newtime;
	__time64_t long_time;

	_time64( &long_time );           // Get time as 64-bit integer.
	_localtime64_s(&newtime, &long_time ); // C4996

	SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, dir);
	//_stprintf_s(dir, MAX_PATH, _T("c:"));

	_stprintf_s(dir + _tcsclen(dir), MAX_PATH-_tcsclen(dir), _T("\\js\\"));
	_tmkdir(dir);

	_stprintf_s(dir + _tcsclen(dir), MAX_PATH-_tcsclen(dir), _T("%.4d%.2d%.2d\\")
		, newtime.tm_year+1900
		, newtime.tm_mon+1
		, newtime.tm_mday);
	_tmkdir(dir);

	_stprintf_s(dir + _tcsclen(dir), MAX_PATH-_tcsclen(dir), _T("%.2d%.2d%.2d.jpg")
		, newtime.tm_hour
		, newtime.tm_min
		, newtime.tm_sec);

	FILE* fp;
	//fp = fopen(dir, "wb");
	_tfopen_s(&fp, dir, _T("wb"));
	if (fp != NULL)
	{
		fwrite((const void*)(pBuff), (DWORD)ulnSize.QuadPart, 1, fp);
		fclose(fp);
	}

	TRACE("-------------ulnSize = %ld, pBuff = %p\n", ulnSize.QuadPart, pBuff);

	delete pBuff;
	pIStream->Release();
	capImage.ReleaseDC();

	////////////////////////////////////////////////////////////////////////////////////
	// camera image capture


	for (int i = 0; i < 10; i++)
	{
		TCHAR name[256], ver[256];
		if (capGetDriverDescription(i, name, 256, ver, 256))
		{
			TRACE(_T("%d, %s, %s\n"), i, name, ver);
		}
	}




    // Create capture window
	int capture_w = 640;
	int capture_h = 480;

	HWND hWndCap = capCreateCaptureWindow(
        NULL, WS_CHILD,
        0, 0, capture_w, capture_h,
		AfxGetMainWnd()->m_hWnd /*hWnd*/, 1);
    if (hWndCap == NULL)
    {
        fprintf(stderr, "Couldn't create capture window.\n");
        return 1;
    }
 
    // Check to see if a specific camera was selected on the
    // on the command line. If a valid integer was not provided
    // atoi returns 0. Device 0 is the default camera.
    int capture_device;
    capture_device = 0; //atoi(lpCmdLine);
 
    // Connect to capture driver and set resolution
	int retryCount = 0;
	BOOL cap_result;
    while (!(cap_result = capDriverConnect(hWndCap, capture_device)) && retryCount < 10)
	{
		retryCount++;
	}

	if (!cap_result)
    {
        fprintf(stderr, "Couldn't open capture device.\n");
        if (hWndCap) ::DestroyWindow(hWndCap);
        return 1;
    }
    DWORD dwSize = capGetVideoFormatSize(hWndCap);
    LPBITMAPINFO lpbi = (LPBITMAPINFO)malloc(dwSize);
    capGetVideoFormat(hWndCap, lpbi, dwSize);
    lpbi->bmiHeader.biWidth = capture_w;
    lpbi->bmiHeader.biHeight = capture_h;
    capSetVideoFormat(hWndCap, lpbi, dwSize);
    free(lpbi);
 
	capGrabFrame(hWndCap);
	_stprintf_s(dir + _tcsclen(dir) - 4, MAX_PATH-_tcsclen(dir), _T("_cam.jpg"));
	capFileSaveDIB(hWndCap, dir);




#if 0
    // Initialise and start video preview
    if (preview_video > 0)
    {
        // Set up video preview
        capPreviewRate(hWndCap, 1); // rate in ms
        capPreview(hWndCap, TRUE);
        ShowWindow(hWndCap, SW_SHOW);
 
        //Show main window
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
    }
 
    // Set timer to trigger snapshot
    SetTimer(hWnd, 1, time_delay, NULL);
 
    // Message loop
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
 
    // Tidy up video capture stuff
    KillTimer(hWnd, 1);
#endif
	capPreview(hWndCap, FALSE);
    capDriverDisconnect(hWndCap);
    if (hWndCap) ::DestroyWindow(hWndCap);
 
 

	return true;
}

#include "StdAfx.h"
#include "SimpleFtp.h"
#include <math.h>
#include <io.h>
#include <direct.h>
#include <strsafe.h>

#pragma comment(lib, "wininet.lib")

#ifndef PUMP_MESSAGES
#define PUMP_MESSAGES() \
	do \
{ \
	MSG msg; \
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) \
{ \
	::TranslateMessage(&msg); \
	::DispatchMessage(&msg); \
} \
	LONG lIdle = 0; \
	while (AfxGetApp()->OnIdle(lIdle ++)); \
}while(0)
#endif

CSimpleFtp::CSimpleFtp(void)
{
	hSession = hFtpConn = hCloseEvent = NULL;
	hCallWnd = NULL;
	hCloseEvent = CreateEvent( NULL,  TRUE, FALSE, NULL);
	bUtf8 = TRUE;
	nPort = 0;
	bPassive = FALSE;
}

CSimpleFtp::~CSimpleFtp(void)
{
	LogOut();
}

//�α���
BOOL CSimpleFtp::Login(CString strIP, CString strID, CString strPasswd, UINT nPort, HWND hCallWnd, BOOL bUtf8, BOOL bPassive)
{
	if(IsConnected() == TRUE)//FTP ������ �� ����Ǿ�������
		LogOut();//�α׾ƿ�
	

	this->strIP = strIP; this->strID = strID; this->strPasswd = strPasswd; this->nPort = nPort;
	this->hCallWnd = hCallWnd;//�������� ����(���۵�����ũ��, �����ð�, �ӵ�)�� ���� ������ �ڵ�(�޽��� ���ۿ� �̿�)
	this->bUtf8 = bUtf8;//FTP ������ UTF8�� �����ϴ� ���
	this->bPassive = bPassive;

	hSession = InternetOpen(NULL,INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);//���� ����
	if(hSession == NULL) 
		return FALSE;
	
	INTERNET_STATUS_CALLBACK iscCallback;// Holder for the callback
	iscCallback = InternetSetStatusCallback(hSession, (INTERNET_STATUS_CALLBACK)CallMaster);//CallMaster�� �ݹ��Լ� ����

	if(bPassive == TRUE)//�нú� ����̸�
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , (DWORD_PTR)this);//�нú� ���
	else//�нú� ��尡 �ƴϸ�
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, 0, (DWORD_PTR)this);//Active ���
	
	if(hFtpConn == NULL)
	{
		//FTP���ῡ �����ϸ�
		TRACE(_T("���ӽ���"));
		return FALSE;
	}

	return TRUE;//FTP���� ���� ����
}

//�ð��������� ������ ������ ����ڰ� �Է��� ������ ������ �ٽ� �α��ν�Ų��
BOOL CSimpleFtp::Login()
{
	if(IsConnected() == TRUE)//FTP ������ �� ����Ǿ�������
		return TRUE;

	if(strIP.GetLength() == 0 || strID.GetLength() == 0 || strPasswd.GetLength() == 0 || nPort == 0)
		return FALSE;

	hSession = InternetOpen(NULL,INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);//���� ����
	if(hSession == NULL) 
		return FALSE;

	INTERNET_STATUS_CALLBACK iscCallback;// Holder for the callback
	iscCallback = InternetSetStatusCallback(hSession, (INTERNET_STATUS_CALLBACK)CallMaster);//CallMaster�� �ݹ��Լ� ����

	if(bPassive == TRUE)//�нú� ����̸�
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , (DWORD_PTR)this);//�нú� ���
	else//�нú� ��尡 �ƴϸ�
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, 0, (DWORD_PTR)this);//Active ���

	if(hFtpConn == NULL)
	{
		//FTP���ῡ �����ϸ�
		InternetCloseHandle(hSession);
		AfxMessageBox(_T("FTP���� ���ӽ���"));
		return FALSE;
	}

	return TRUE;//FTP���� ���� ����

}

//�α� �ƿ�
void CSimpleFtp::LogOut()
{
	if(hCloseEvent)
	{
		SetEvent(hCloseEvent);//�������� �����Ͱ� �ִٸ� ���� ����
		CloseHandle(hCloseEvent);
		hCloseEvent = NULL;
	}

	if(hFtpConn)
	{
		InternetCloseHandle(hFtpConn);//FTP �ڵ� ����
		hFtpConn = NULL;
	}
	if(hSession)
	{
		InternetCloseHandle(hSession);//���� ����
		hSession = NULL;
	}
	
	hCallWnd = NULL;
}

//FTP������ ����Ǿ������� TRUE, �ƴϸ� FALSE
BOOL CSimpleFtp::IsConnected()
{
	DWORD dwState = 0;
	
	if(hFtpConn == NULL || hSession == NULL)//�����ڵ��� NULL�� ���
		return FALSE;//����ȵ�

	return TRUE;
}

//strRemoteFilePath�� strLocalFilePath ��ο� ����(���ϸ� �ٿ�ε�)
BOOL CSimpleFtp::DownloadFile(CString strRemoteFilePath, CString strLocalFilePath, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return FALSE;
		}
	}
	TransferInfo_In st_TransferInfo_In;//���۱⺻����(�������� ���ϸ�, ���۽��۽ð�, ��������ð� ������ �������� �� ũ�� ���� ���۵� ������ ũ��
	TransferInfo_Out st_TransferInfo_Out;//���۱⺻������ �̿��Ͽ� ���ۼӵ�, �����ð� ���� �߰������� ����

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));


	st_TransferInfo_In.dwStartTime = ::GetTickCount();//���۽��۽ð�
	_tcscpy_s(st_TransferInfo_In.szFilePath, strLocalFilePath);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	FILE* fp = NULL;
	fopen_s(&fp, CT2A(strLocalFilePath), bResume ? "ab" : "wb");//�̾�ޱ� �Ǵ� �Ϲ� �ٿ�ε� ���ϻ���
	if(!fp)
	{
		TRACE(_T("���� ���� ����"));
		return FALSE;
	}

	if(GetRemoteFileSize(strRemoteFilePath, st_TransferInfo_In.nTotFileSize) == FALSE)//������ ũ�⸦ ����
		return FALSE;

	if(bResume)
	{
		_fseeki64(fp, 0, SEEK_END);
		UINT64 nFileSize = _ftelli64(fp);//��������ũ��

		st_TransferInfo_In.nTransferSize = nFileSize;//���� ����ũ��
		st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
		if(hCallWnd != NULL)//Login���� hCallWnd�� �����Ǹ�
		{
			//���� �ٿ�ε� ������ ������
			GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
			::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);	
			PUMP_MESSAGES();
		}

		if((st_TransferInfo_In.nTotFileSize - nFileSize) <=0 )//���� ����ũ�Ⱑ FTP������ ���Ϻ��� ũ�ٸ�
		{
			fclose(fp);
			return TRUE;//�ٿ�ε� ���� �ʿ� ����
		}

		if(ReadyDownloadResume(nFileSize) == FALSE)//�̾�ޱ� ��� �غ�
		{
			//�غ� ����
			fclose(fp);
			fopen_s(&fp, CT2A(strLocalFilePath), "wb");//�ٿ�ε� ���ϻ���
			if(!fp)
				return FALSE;
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//FTP������ ���� ����
	HINTERNET hFile = FtpOpenFileA(hFtpConn, GetSendString(strRemoteFilePath), GENERIC_READ, INTERNET_FLAG_TRANSFER_BINARY, 0);
	if(hFile == NULL)
		return FALSE;//����
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//�ٿ�ε� �������丮�� ������ �����ϰ� ������ ����
	CString strDownloadDir = GetParentDirFromPath(strLocalFilePath);
	if(strDownloadDir.GetLength() > 0)
	{
		//��θ� ������ ���ϸ��� ���
		if(CreateLocalDir(strDownloadDir) <= 0)//�ٿ�ε� ���� ���丮 ����
			return FALSE;//���丮 ���� ����
	}
	else
	{
		//��� ���� ���ϸ� �ִ� ���
		strDownloadDir = GetWorkingDir();
		strLocalFilePath = strDownloadDir + _T("\\") + strLocalFilePath;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//���� �ٿ�ε�
	BYTE Buff[MAX_BUFFER_SIZE]={0,};
	DWORD dwRead = 0;
	UINT64 nTotRead = st_TransferInfo_In.nTransferSize;
	do
	{
		if(InternetReadFile(hFile, Buff, MAX_BUFFER_SIZE, &dwRead) == FALSE)//���ϵ����͸� �о��
		{
			fclose(fp);
			InternetCloseHandle(hFile);
			ShowLastErrorMsg();
			return FALSE;
		}
		
		fwrite(Buff, dwRead, 1, fp);

		nTotRead += dwRead;
		st_TransferInfo_In.nTransferSize = nTotRead;
		st_TransferInfo_In.dwCurrentTime = ::GetTickCount();

		if(hCallWnd != NULL)//Login���� hCallWnd�� �����Ǹ�
		{
			//���� �ٿ�ε� ������ ������
			GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
			::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);	
			PUMP_MESSAGES();
		}

		if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//������ ����
		{
			InternetCloseHandle(hFile);
			fclose(fp);
			return FALSE;
		}

	}while(dwRead > 0);

	fclose(fp);
	InternetCloseHandle(hFile);


	return TRUE;
}


//�������丮 �����Ͽ� �ٿ�ε��Ѵ�
//FTP������ "/123/456/789" ��� ���丮�� �����Ҷ�
//strRemoteDir = "/123": , strLocalDir = "c:\SimpleFtp" ���
//���� ���õ�ũ�� ������ ����� c:\SimpleFtp\456\789 ��
BOOL CSimpleFtp::DownloadWithSubDir(CString strRemoteDir, CString strLocalDir, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return FALSE;
		}
	}

	FILE_LIST down_list;
	FILE_LIST::iterator itr;
	
	if(GetRemoteFileListWithSubDir(strRemoteDir, down_list) == FALSE)//�������丮 �����Ͽ� �ٿ�ε����� down_list�� ����
		return FALSE;
	
	//���ð�θ��� �������� '\'�� ������ �����Ѵ�
	if(strLocalDir.GetAt(strLocalDir.GetLength()-1) == _T('\\'))//����ڰ� �Է��� ��������� �������� '\'�� ������
		strLocalDir = strLocalDir.Left(strLocalDir.GetLength()-1);//'\'�� ''(����)���� ��ȯ

	UINT64 nTotRead = 0;//��ü �ٿ�ε� ������ ũ��
	TransferInfo_In st_TransferInfo_In;//���۱⺻����(�������� ���ϸ�, ���۽��۽ð�, ��������ð� ������ �������� �� ũ�� ���� ���۵� ������ ũ��
	TransferInfo_Out st_TransferInfo_Out;//���۱⺻������ �̿��Ͽ� ���ۼӵ�, �����ð� ���� �߰������� ����

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));

	st_TransferInfo_In.dwStartTime = ::GetTickCount();
	st_TransferInfo_In.nTotFileSize = GetTotFileSize(down_list);//�ٿ�ε� ���� �������� ��ũ�⸦ ����

	//�ٿ�ε� ����� ��ȸ�ϸ鼭 ���ϴٿ�ε�
	for(itr = down_list.begin(); itr != down_list.end(); itr++)
	{
		if(!IsConnected())//������ ������ �Ǿ� ���� �ʴٸ�
			return FALSE;//�ٿ�ε� ����

		CString strRemotePath = itr->second.szFilePath;
		CString strLocalFilePath;

		if(strRemoteDir != _T("/"))
			strRemotePath.Replace(strRemoteDir, _T(""));//����ڰ� ������ ���ݵ��丮�� �������丮�� �ٿ�ε�

		strRemotePath.Replace(_T("/"), _T("\\"));
		strLocalFilePath = strLocalDir + _T("\\") + strRemotePath;//�ٿ�ε忡 �ʿ��� ���ÿ� �������ϴ� ���丮
		strLocalFilePath.Replace(_T("\\\\"), _T("\\"));

		if(itr->second.isDir == TRUE)//���丮����?
		{
			if(CreateLocalDir(strLocalFilePath) == FALSE)//���丮 ����(�������丮�� ���ٸ� ������ ����)
			{
				TRACE(_T("�������� ���� ����"));
				return FALSE;
			}
			continue;
		}
		else
		{
			if(CreateLocalDir(GetParentDirFromPath(strLocalFilePath)) == FALSE)//���丮 ����(�������丮�� ���ٸ� ������ ����)
			{
				TRACE(_T("�������� ���� ����"));
				return FALSE;
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		FILE* fp = NULL;
		fopen_s(&fp, CT2A(strLocalFilePath), bResume ? "ab" : "wb");//�̾�ޱ� �Ǵ� �Ϲ� �ٿ�ε� ���ϻ���
		if(!fp)
		{
			TRACE(_T("���� ���� ����"));
			return FALSE;
		}
		
		if(bResume)//�̾�ޱ�?
		{
			//�̾�ޱ���
			INT64 nReadOffset = 0;
			fseek(fp, 0, SEEK_END);
			UINT64 nFileSize = _ftelli64(fp);
			if(ReadyDownloadResume(nFileSize) == FALSE)//�̾�ޱ� ��� �غ�
			{
				//�̾�ޱ� �غ� ����(�������� ����)
				fclose(fp);
				fopen_s(&fp, CT2A(strLocalFilePath), "wb");//ó������ �ٽ� �ٿ�ε�
				if(!fp)
					return FALSE;
				
				bResume = FALSE;
			}
			else
			{
				nTotRead += nFileSize;
				st_TransferInfo_In.nTransferSize = nTotRead;
				st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
				nReadOffset = itr->second.nFileSize - nFileSize;//FTP���� ����ũ�� - ��������ũ��
				if(nReadOffset <= 0)
				{
					fclose(fp);
					if(hCallWnd != NULL)
					{
						GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
						::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);//�ٿ�ε� ������ �θ������� ������
						PUMP_MESSAGES();
					}
					continue;
				}
			}
			
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		_tcscpy_s(st_TransferInfo_In.szFilePath, strLocalFilePath);
		//FTP���� ����
		HINTERNET hFile = FtpOpenFileA(hFtpConn, GetSendString(itr->second.szFilePath), GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_TRANSFER_BINARY, 0);
		if(hFile == FALSE)
			return FALSE;

		
		BYTE Buff[MAX_BUFFER_SIZE]={0,};
		DWORD dwRead = 0;
		do
		{
			if(InternetReadFile(hFile, Buff, MAX_BUFFER_SIZE, &dwRead) == FALSE)//�����κ��� �����͸� �д´�
			{
				fclose(fp);
				InternetCloseHandle(hFile);
				ShowLastErrorMsg();
				return FALSE;
			}
			fwrite(Buff, dwRead, 1, fp);

			nTotRead += dwRead;
			st_TransferInfo_In.nTransferSize = nTotRead;//������� �� �ٿ�ε��� �������� ũ��
			st_TransferInfo_In.dwCurrentTime = ::GetTickCount();//���ݰ��� �������� �ð�

			if(hCallWnd != NULL)
			{
				GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
				::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);//�ٿ�ε� ������ �θ������� ������
				PUMP_MESSAGES();
			}

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//������ ����
			{
				fclose(fp);
				InternetCloseHandle(hFile);
				return FALSE;
			}

		}while(dwRead > 0);


		InternetCloseHandle(hFile);
		fclose(fp);
	}

	return TRUE;
}


//������ ���ε��Ѵ�
BOOL CSimpleFtp::UploadFile(CString strLocalFilePath, CString strRemoteFilePath, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return FALSE;
		}
	}

	int nPos = strLocalFilePath.Find(_T("\\"));
	if(nPos < 0)//'\'�� �� ã����
		strLocalFilePath = GetWorkingDir() + _T("\\") + strLocalFilePath;//��ξ��� ���ϸ� �ִ� ��� ���������� �ִ� ��θ� ����Ʈ ��η� �Ѵ�

	if(strRemoteFilePath != _T("/"))
	{
		CString strRemoteDir = GetParentDirFromPath(strRemoteFilePath);

		strRemoteDir.Replace(_T("\\"), _T("/"));
		strRemoteDir.Replace(_T("//"), _T("/"));

		if(CreateRemoteDir(strRemoteDir) <= 0)
		{
			AfxMessageBox(_T("���丮 ���� ����"));
			return FALSE;
		}
	}

	
	HINTERNET hFile = NULL;
	UINT64 nStartFilePointer = 0;
	if(bResume)
	{
		if(GetRemoteFileSize(strRemoteFilePath, nStartFilePointer) == FALSE)//����ũ�� ���ϱ�
			nStartFilePointer = 0;

		if(nStartFilePointer > 0)//������ ������ 0���� ũ�ٸ�
			ReadyUploadResume(strRemoteFilePath, nStartFilePointer);//�̾� �ø��� �غ�
		else
			bResume = FALSE;//�̾�ø��� ����
	}
	
	TransferInfo_In st_TransferInfo_In;//���۱⺻����(�������� ���ϸ�, ���۽��۽ð�, ��������ð� ������ �������� �� ũ�� ���� ���۵� ������ ũ��
	TransferInfo_Out st_TransferInfo_Out;//���۱⺻������ �̿��Ͽ� ���ۼӵ�, �����ð� ���� �߰������� ����

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));

	FILE* fp = NULL;
	fopen_s(&fp, CT2A(strLocalFilePath), "rb");
	if(!fp)
	{
		InternetCloseHandle(hFile);
		TRACE(_T("���� ���� ����"));
		return FALSE;
	}

	_fseeki64(fp, 0, SEEK_END);
	st_TransferInfo_In.nTotFileSize = _ftelli64(fp);//���ε��� ������ ũ�⸦ ����
	st_TransferInfo_In.dwStartTime = ::GetTickCount();//���۽��� �ð�
	if(bResume)
	{
		_fseeki64(fp, nStartFilePointer, SEEK_SET);//���� �����͸� �̾�ø� �������� �̵�
	}
	else
		_fseeki64(fp, 0, SEEK_SET);//���� �����͸� ���������� �̵�

	if(bResume)
	{
		st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
		st_TransferInfo_In.nTransferSize = nStartFilePointer;
		if(hCallWnd)
		{
			GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
			::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
			PUMP_MESSAGES();
		}
		
		if((st_TransferInfo_In.nTotFileSize - nStartFilePointer) <= 0)//������ ������ ���ε��� �������Ϻ��� ũ�ٸ�
			return TRUE;//�������ۿϷ�(���������� �ʿ����)
	}

	hFile = FtpOpenFileA(hFtpConn, GetSendString(strRemoteFilePath), GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_TRANSFER_BINARY, 0);

	if(hFile == NULL)
	{
		ShowLastErrorMsg();
		return FALSE;
	}

	BYTE Buff[MAX_BUFFER_SIZE]={0,};
	DWORD dwWrite = 0;
	UINT64 nTotSend = nStartFilePointer;
	while(!feof(fp))//���������� �� ���������� �ݺ�
	{
		_tcscpy_s(st_TransferInfo_In.szFilePath, strLocalFilePath);//�������� ���ϸ�(�������)
		size_t nReadSize = fread(Buff, 1, MAX_BUFFER_SIZE, fp);

		if(nReadSize == 0)
			break;

		UINT dwSend = 0;
		do
		{	
			//FTP������ ���ε�
			if(InternetWriteFile(hFile, Buff+dwSend, nReadSize-dwSend, &dwWrite) == FALSE)
			{
				InternetCloseHandle(hFile);
				fclose(fp);
				ShowLastErrorMsg();
				return FALSE;
			}
			nTotSend += dwWrite;//�� ���۵� ũ��
			dwSend += dwWrite;//���� ������ ���۵� ũ��
			*Buff += dwSend;//MAX_BUFFER_SIZE

			st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
			st_TransferInfo_In.nTransferSize = nTotSend;

			if(hCallWnd)
			{
				GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
				::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
				PUMP_MESSAGES();
			}

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//������ ����
			{
				InternetCloseHandle(hFile);
				fclose(fp);
				return FALSE;
			}
		}while(dwSend < nReadSize);
	}

	InternetCloseHandle(hFile);
	fclose(fp);

	return TRUE;
}

//�������丮 �����Ͽ� ���ε�
BOOL CSimpleFtp::UploadWithSubDir(CString strLocalDir, CString strRemoteDir, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return FALSE;
		}
	}

	if(strLocalDir.GetLength() == 0)
		return FALSE;

	if(strLocalDir.GetAt(strLocalDir.GetLength()-1) == _T('\\'))//��������� �������� '/'�̸�
		strLocalDir = strLocalDir.Left(strLocalDir.GetLength() - 1);//������ '/' ����


	if(strRemoteDir.GetLength() == 0)
		return FALSE;

	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//��������� �������� '/'�̸�
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1);//������ '/' ����

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));


	FILE_LIST upload_list;
	FILE_LIST::iterator itr;
	TransferInfo_In st_TransferInfo_In;//���۱⺻����(�������� ���ϸ�, ���۽��۽ð�, ��������ð� ������ �������� �� ũ�� ���� ���۵� ������ ũ��
	TransferInfo_Out st_TransferInfo_Out;//���۱⺻������ �̿��Ͽ� ���ۼӵ�, �����ð� ���� �߰������� ����
	UINT64 nTotSend = 0;

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));

	GetLocalFileListWithSubDir(strLocalDir, upload_list);
	st_TransferInfo_In.nTotFileSize = GetTotFileSize(upload_list);
	st_TransferInfo_In.dwStartTime = ::GetTickCount();

	//���ϸ���� ��ȸ�ϸ� ���ε�
	for(itr = upload_list.begin(); itr != upload_list.end(); itr++)
	{
		if(!IsConnected())//���ε� �߿� ������ ������ ����ٸ�
			return FALSE;//���ε� ����

		CString strSubDir, strLocalFilePath, strRemoteFilePath;
		_tcscpy_s(st_TransferInfo_In.szFilePath, itr->second.szFilePath);//�������� ���ϸ�(�������)

		if(itr->second.isDir)
			strSubDir = itr->second.szFilePath;
		else
		{
			strSubDir = GetParentDirFromPath(itr->second.szFilePath);
		}
		strSubDir.Replace(strLocalDir, _T(""));
		strSubDir.Replace(_T("\\"), _T("/"));
		CString strCreateDir = strRemoteDir + strSubDir;
		strCreateDir.Replace(_T("//"), _T("/"));
		if(CreateRemoteDir(strCreateDir) == FALSE)
		{
			CString strMsg;
			strMsg.Format(_T("%s ���丮 ���� ����"), strCreateDir);
			AfxMessageBox(strMsg);
			return FALSE;
		}

		if(itr->second.isDir == TRUE)
			continue;
		else
		{
			strLocalFilePath = itr->second.szFilePath;
			strLocalFilePath.Replace(strLocalDir, _T(""));
			strLocalFilePath.Replace(_T("\\"), _T("/"));
		}

		strRemoteFilePath = strRemoteDir + strLocalFilePath;
		strRemoteFilePath.Replace(_T("//"), _T("/"));

		HINTERNET hFile = NULL, hFtpCommand = NULL;
		UINT64 nStartFilePointer = 0;
		
		if(bResume)
		{
			//�̾�ø��� �����
			INT64 nWriteOffset = 0;
			if(GetRemoteFileSize(strRemoteFilePath, nStartFilePointer) == FALSE)//����ũ�� ���ϱ�
				nStartFilePointer = 0;//FTP������ ������ ���⶧����(���� ������ ���� ����)

			
			nWriteOffset = itr->second.nFileSize - nStartFilePointer;//��������ũ�� - FTP���� ����ũ��
			if(nWriteOffset <= 0)//������ ������ �������Ϻ��� ũ�ٸ�
			{
				st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
				nTotSend += nStartFilePointer;//�̾�ޱ� �̹Ƿ� �̹� ������ �����ͷ� ���
				st_TransferInfo_In.nTransferSize = nTotSend;
				if(hCallWnd)
				{
					GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
					::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
					PUMP_MESSAGES();
				}
				continue;//�������Ϸ�
			}

			if(nStartFilePointer > 0)//������ ������ 0���� ũ�ٸ� 
				ReadyUploadResume(strRemoteFilePath, nStartFilePointer);//�̾�ø��� �غ�
			
		}
		//������ ������ ����(�̾�ø���� �ƴϵ� FTP������ ������ ����)
		hFile = FtpOpenFileA(hFtpConn, GetSendString(strRemoteFilePath), GENERIC_WRITE, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_TRANSFER_BINARY, 0);
		
		if(hFile == NULL)
		{
			ShowLastErrorMsg();
			return FALSE;
		}

		FILE* fp = NULL;
		fopen_s(&fp, CT2A(itr->second.szFilePath), "rb");
		if(!fp)
		{
			InternetCloseHandle(hFile);
			TRACE(_T("���� ���� ����"));
			return FALSE;
		}

		if(bResume)
		{
			_fseeki64(fp, nStartFilePointer, SEEK_SET);//���� �����͸� �̾�ø� �������� �̵�
			nTotSend += nStartFilePointer;//�̾�ޱ� �̹Ƿ� �̹� ������ �����ͷ� ���
			st_TransferInfo_In.nTransferSize = nTotSend;
		}
		else
			_fseeki64(fp, 0, SEEK_SET);//���� �����͸� ���������� �̵�

		BYTE Buff[MAX_BUFFER_SIZE]={0,};
		DWORD dwWrite = 0;
		while(!feof(fp))
		{
			size_t nReadSize = fread(Buff, 1, MAX_BUFFER_SIZE, fp);
			if(nReadSize == 0)
				break;

			UINT dwSend = 0;
			do
			{	
				//���ε�, �� ���� �����Ͱ� �ִٸ� �ݺ��ؼ� �������� �ڵ带 �ۼ��ϱ� ������
				//InternetWriteFile�� ���δܿ��� ������ ũ�⸸ŭ �����͸� �׻� �����ϴ� �� �ϴ�
				if(InternetWriteFile(hFile, Buff+dwSend, nReadSize-dwSend, &dwWrite) == FALSE)
				{
					InternetCloseHandle(hFile);
					fclose(fp);
					ShowLastErrorMsg();
					return FALSE;
				}

				nTotSend += dwWrite;//�� ���ε� ������ ũ��
				dwSend += dwWrite;//���� ������ ���ε� ������ ũ��, 

				st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
				st_TransferInfo_In.nTransferSize = nTotSend;

				if(hCallWnd)
				{
					GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
					::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
					PUMP_MESSAGES();
				}

				if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//������ ����
				{
					InternetCloseHandle(hFile);
					fclose(fp);
					return FALSE;
				}
			}while(dwSend < nReadSize);
		}
		InternetCloseHandle(hFile);
		fclose(fp);	
	}


	return TRUE;
}


//�������丮 ������ FTP ������ ���ϸ���� ����
//FtpFileFind�� ���ȣ���� ����� �� ���� �Ӽ������� �Ʒ��� ���� �ۼ�
BOOL CSimpleFtp::GetRemoteFileListWithSubDir(__in CString strRemoteDir, __out FILE_LIST& file_list)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return FALSE;
		}
	}

	//���� ���丮�� ����ִ� ���� �� ���丮 ����� ����(�������丮 ������)
	if(GetRemoteFileList(strRemoteDir, file_list) == FALSE)
		return FALSE;

	//�������丮 �����Ͽ� ���� �� ���丮 ����� ����
	FILE_LIST::iterator itr;
	for(itr = file_list.begin(); itr != file_list.end(); itr++)
	{
		if(itr->second.isDir == TRUE)
		{
			if(GetRemoteFileList(itr->second.szFilePath, file_list) == FALSE)
				return FALSE;
		}
	}

	return TRUE;
}



//���� �� ���丮 ����� ����(�������丮 ������)
//���� : TRUE, ���� : FALSE
BOOL CSimpleFtp::GetRemoteFileList(__in CString strRemoteDir, __out FILE_LIST& file_list)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return FALSE;
		}
	}

	if(strRemoteDir.GetLength() == 0)
		return FALSE;

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//����ڰ� �Է��� ��������� �������� '\'�� ������
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1); //'/'�� ����


	WIN32_FIND_DATAA find_data;
	memset(&find_data, 0, sizeof(WIN32_FIND_DATAA));
	CString strAddPath;
	
	//�������� ������ ��������θ��� �����Է��ص� ���ϸ���� ���ؿ��� �ݸ� ������ ��FTP�� �������� ���ϸ�ϸ� �����Ѵ�
	if(FtpSetCurrentDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)//��FTP ������ ���丮�� �̵��ϰ� ����Ʈ�� ���ؾ� �Ѵ�
		return FALSE;//���丮 �̵� ����

	//���� �� ���丮 ����� ����
	HINTERNET hFind = FtpFindFirstFileA(hFtpConn, "*", &find_data, INTERNET_FLAG_RELOAD, NULL);
	if(hFind)
	{
		strAddPath = AddRemoteFile(strRemoteDir, find_data, file_list);

		BOOL bFind = FALSE;
		do
		{
			memset(&find_data, 0, sizeof(WIN32_FIND_DATAA));
			bFind = InternetFindNextFileA(hFind, &find_data);
			strAddPath = AddRemoteFile(strRemoteDir, find_data, file_list);

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//������ ����
			{
				InternetCloseHandle(hFind);
				return FALSE;
			}

		}while(bFind);
	}

	InternetCloseHandle(hFind);

	return TRUE;
}


//WIN32_FIND_DATAA�� ������ file_list�� �߰��Ѵ�
//���⼭ ���� �� ���丮���� ���ڼ� ������ ����� �� �ߴµ�
//��FTP�� �����ڵ�(UTF8)�� �������� ������ ��������(FileZilla)�� �������� ProFtp�� �����ڵ常 �����ϴ� ��
//FTP���� ���α׷��� ���� ���ڼ��� ���� �ٸ�. ���� �ΰ��� ��츦 ��� �����ϱ� ����
//Login�Ҷ� Utf8���� �����ϵ��� �Ͽ���. ��FTP�� ���� bUtf8 = false, �� �� Utf8�� �����ϴ� ������ bUtf8 = true�� ����
CString CSimpleFtp::AddRemoteFile(const CString strCurrentRemoteDir, const WIN32_FIND_DATAA& find_data, FILE_LIST& file_list)
{
	FileInfo st_FileInfo;
	memset(&st_FileInfo, 0, sizeof(FileInfo));
	CString strName;//���丮 �Ǵ� ���ϸ�

	//�����ڵ� ������ �����̶��
#ifdef UNICODE
	if(bUtf8 == TRUE)
		strName = CA2W(find_data.cFileName, CP_UTF8);//���丮 �Ǵ� ���ϸ�
	else
		strName = CA2W(find_data.cFileName);
#else//��Ƽ����Ʈ ������ �����̶��
	if(bUtf8 == TRUE)
	{
		CStringW strTemp =  CA2W(find_data.cFileName, CP_UTF8);//���丮 �Ǵ� ���ϸ�
		strName = CW2A(strTemp);
	}
	else
		strName = find_data.cFileName;
#endif

	if(strName.GetLength() == 0)
		return _T("");

	if(strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
		return _T("");

	if(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		st_FileInfo.isDir = TRUE;
		if(strName.GetAt(strName.GetLength()-1) == _T('/'))//����ڰ� �Է��� ��������� �������� '/'�� ������
			strName = strName.Left(strName.GetLength() - 1);
		strName.Replace(_T("\\"), _T("/"));
		strName.Replace(_T("//"), _T("/"));
	}
	else
		st_FileInfo.isDir = FALSE;

	st_FileInfo.nFileSize = (find_data.nFileSizeHigh * (MAXDWORD + (__int64)1)) + find_data.nFileSizeLow;//����ũ��
	FileTimeToSystemTime(&find_data.ftCreationTime, &st_FileInfo.stCreattionTime);//������(FTP���� ���� ���ϴ� ��)
	FileTimeToSystemTime(&find_data.ftLastAccessTime, &st_FileInfo.stLastAccessTime);//����������(FTP���� ���� ���ϴ� ��)
	FileTimeToSystemTime(&find_data.ftLastWriteTime, &st_FileInfo.stLastWriteTime);//����������(FTP����)

	CString strRemoteFilePath = strCurrentRemoteDir + _T("/") + strName;
	if(strRemoteFilePath.GetAt(0) != _T('/'))
		strRemoteFilePath = _T("/") + strRemoteFilePath;
	strRemoteFilePath.Replace(_T("//"), _T("/"));
	_tcscpy_s(st_FileInfo.szFilePath, strRemoteFilePath);
	file_list.insert(pair<CString, FileInfo>(st_FileInfo.szFilePath, st_FileInfo));//�� �����Ϳ� �߰�

	return st_FileInfo.szFilePath;
}

//FTP������ ����ũ�⸦ ����
//��FTP������ FtpGetFileSize ���� Ʋ������ �־� �Ʒ�ó�� �Ѵ�
BOOL CSimpleFtp::GetRemoteFileSize(__in CString strRemoteFilePath, __out UINT64& nRemoteFileSize)
{
	CString strRemoteDir = GetParentDirFromPath(strRemoteFilePath), strRemoteFileName = GetFileNameFromPath(strRemoteFilePath);
	nRemoteFileSize = 0;

	
	DWORD dwLow = 0, dwHigh = 0;

	FILE_LIST file_list;
	FILE_LIST::iterator itr;
	GetRemoteFileList(strRemoteDir, file_list);//���ϸ���� ���Ѵ�
	itr = file_list.find(strRemoteFilePath);
	if(itr != file_list.end() && itr->second.isDir == FALSE)
		nRemoteFileSize = itr->second.nFileSize;//����ũ��
	else
		return FALSE;

	return TRUE;
}


//file_list�� ����� ����� �� ����ũ�⸦ ����
__out UINT64 CSimpleFtp::GetTotFileSize(__in FILE_LIST& file_list)
{
	unsigned __int64 nTotFileSize = 0;
	FILE_LIST::iterator itr;
	for(itr = file_list.begin(); itr != file_list.end(); itr++)
	{
		if(itr->second.isDir == FALSE) //������ ���
			nTotFileSize += itr->second.nFileSize;
	}

	return nTotFileSize;
}

//�������丮 �����Ͽ� ���丮�� �����Ѵ�
//���� strDirPath�� c:\123\456\789��� c:\, c:\123, c:\123\456, c:\123\456\789 ������ �����Ѵ�
int CSimpleFtp::CreateLocalDir(CString strDirPath)
{
	strDirPath.Replace(_T("\\\\"), _T("\\"));
	if(strDirPath.GetLength() == 0)
		return -1;
	
	if(strDirPath.GetAt(strDirPath.GetLength()-1) == _T('\\'))//����ڰ� �Է��� ��������� �������� '\'�� �� ������
		strDirPath = strDirPath.Left(strDirPath.GetLength()-1);//'\'�� ''(����)���� ��ȯ


	BOOL bSuccess = TRUE;

	//strDirPath ���ڿ��� �߰��� �ִ� ���� ����
	//���� c:\123\456\789 ��� c:\, c:\123, c:\123\456 ���� ����
	for(int i = 0; i < strDirPath.GetLength(); i++)
	{
		if(strDirPath.GetAt(i) == _T('\\'))
		{
			CString strMakeDir = strDirPath.Left(i);
			if(_taccess_s(strMakeDir, 0) == ENOENT)//���丮�� �������� ������
			{
				if(_tmkdir(strMakeDir) == ENOENT)//���丮 ������ �õ�
				{
					//���丮 ������ �����ϸ�
					bSuccess = FALSE;
					return bSuccess;
				}
			}
		}
	}

	if(_tmkdir(strDirPath) == ENOENT)//���� ���丮(c:\123\456\789) ����
		bSuccess = FALSE;//����

	return bSuccess;
}

//Ư�� ���丮 ���ϸ�� ���ϱ�
UINT64 CSimpleFtp::GetLocalFileListWithSubDir(CString strDirPath, FILE_LIST& file_list) 
{
	static UINT64 m_TotFileSize = 0;//����Լ��� ����� ���ϱ� ������ static���� �ؾ� ���� ���� ������ �� �ִ�
	try
	{
		CFileFind m_FileFind;
		m_FileFind.FindFile(strDirPath+ _T("\\*.*"));

		BOOL bContinue = TRUE;


		FileInfo st_FileInfo;
		while(bContinue)
		{
			memset(&st_FileInfo, 0, sizeof(FileInfo));
			bContinue = m_FileFind.FindNextFile();

			CString strFileName = m_FileFind.GetFileName(); //������ �ִ� ���
			CString strFullPath = strDirPath + _T("\\") + strFileName;


			// '.', '..'�� ����
			if(m_FileFind.IsDots())
				continue;

			CTime m_Time;
			m_FileFind.GetCreationTime(m_Time);
			m_Time.GetAsSystemTime(st_FileInfo.stCreattionTime);//������
			m_FileFind.GetLastWriteTime(m_Time);
			m_Time.GetAsSystemTime(st_FileInfo.stLastWriteTime);//���� ������
			m_FileFind.GetLastAccessTime(m_Time);
			m_Time.GetAsSystemTime(st_FileInfo.stLastAccessTime);//���� ������

			//���� ��ü���
			CString strSavePath = strFullPath;
			////////////////////////////////////////////////////////////
			if(m_FileFind.IsDirectory())      // ã�� ������ ���丮�̸�
			{
				if(strSavePath.GetLength() > 0)
					_tcscpy_s(st_FileInfo.szFilePath, strSavePath);

				st_FileInfo.isDir = TRUE;

				// ���� ���丮 �̹Ƿ� ��� ȣ���Ͽ� ���� ���丮�� �̵�
				GetLocalFileListWithSubDir(strDirPath + _T("\\") + strFileName, file_list);
			}
			else
			{
				st_FileInfo.nFileSize = m_FileFind.GetLength();
				m_TotFileSize += st_FileInfo.nFileSize;
				st_FileInfo.isDir = FALSE;
				_tcscpy_s(st_FileInfo.szFilePath, strSavePath);
			}

			file_list.insert(pair<CString, FileInfo>(st_FileInfo.szFilePath, st_FileInfo));//�ʿ� �����Ѵ�
		}

		m_FileFind.Close();
	}
	catch(CFileException* e)
	{
		TCHAR szErr[1024]={0,};
		e->GetErrorMessage(szErr, 1023);
		AfxMessageBox(szErr);
		delete[] e;
		return m_TotFileSize;
	}
	return m_TotFileSize;
}

//���� exe������ ����Ǵ� ��θ� ���ϴ� �Լ�
CString CSimpleFtp::GetWorkingDir()
{
	CString strWorkingDir;
	int nPos = -1;
	TCHAR path[1024]={0,};
	GetModuleFileName(NULL, path, 1024);
	strWorkingDir = path;
	strWorkingDir.MakeReverse();
	nPos = strWorkingDir.Find(_T("\\"), 0);
	strWorkingDir.Delete(0, nPos);
	strWorkingDir.MakeReverse();

	if(strWorkingDir.GetAt(strWorkingDir.GetLength()-1) == _T('\\'))
		strWorkingDir.Left(strWorkingDir.GetLength() - 1);

	return strWorkingDir;
}

//���������� ����
void CSimpleFtp::GetTransferInfo(__in const TransferInfo_In& st_TransferInfo_In, __out TransferInfo_Out& st_TransferInfo_Out)
{
	int nPercent = 0, nDevide = 0;//�������� �����
	DWORD nSpeed =0, nRemainTime = 0;//�ӵ�, �����ð�
	double ElapsedTime = 0;//�帥�ð�
	int nHour = 0, nMinute = 0, nSecond = 0;//�ð�, ��, ��
	
	//�ӵ��� ������ ���̱� ���ؼ� �ʴ� ���ۼӵ��� ���� ���
	if(st_TransferInfo_In.nTransferSize > 1048576)//2^20
		nDevide = 1048576;
	else if(st_TransferInfo_In.nTransferSize > 1024)//2^10
		nDevide = 1024;
	else
		nDevide = 1;
	
	if(st_TransferInfo_In.nTotFileSize > 0.0)
		nPercent = static_cast<int> (((double)st_TransferInfo_In.nTransferSize/(double)st_TransferInfo_In.nTotFileSize)*100);
	ElapsedTime = (st_TransferInfo_In.dwCurrentTime - st_TransferInfo_In.dwStartTime)/1000.0;//������� �帥�ð� ��
	
	if(ElapsedTime >= 0.1)
		nSpeed = static_cast<int> (((st_TransferInfo_In.nTransferSize / nDevide) / ElapsedTime));//�ʴ� �ӵ�(KB,MB)
	else
		nSpeed = 0;

	if(nSpeed > 0)
		nRemainTime = static_cast<int> ((double)(((st_TransferInfo_In.nTotFileSize - st_TransferInfo_In.nTransferSize)/nDevide))/(double)nSpeed);//�����ð�
	
	if(st_TransferInfo_In.nTotFileSize == 0)
		nRemainTime = 0;
		

	nHour = static_cast<int>(floor(nRemainTime/3600.0));
	nMinute = static_cast<int>(floor(nRemainTime/60.0));


	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));
	_tcscpy_s(st_TransferInfo_Out.szFilePath, st_TransferInfo_In.szFilePath);
	st_TransferInfo_Out.nTotFileSize = st_TransferInfo_In.nTotFileSize;//�� ������ ����ũ��
	st_TransferInfo_Out.nTransferSize = st_TransferInfo_In.nTransferSize;//���� ����� ����ũ��
	st_TransferInfo_Out.nSpeed = nSpeed * nDevide;//���ۼӵ�
	st_TransferInfo_Out.nPercent = nPercent;//��������(�����)

	if(nHour > 0)
	{
		//�����ð��� �ð�, ������ ����Ѵ�
		nMinute = static_cast<int>((nRemainTime/3600.0 - floor(nRemainTime/3600.0))*60);
		st_TransferInfo_Out.nRemainHour = nHour;
		st_TransferInfo_Out.nRemainMin = nMinute;
	}
	else if(nHour <= 0 && nMinute > 0)
	{
		//�����ð��� ��, �ʷ� ����Ѵ�
		nMinute = static_cast<int>(floor(nRemainTime/60.0));
		nSecond = static_cast<int>((nRemainTime/60.0 - floor(nRemainTime/60.0))*60);
		st_TransferInfo_Out.nRemainMin = nMinute;
		st_TransferInfo_Out.nRemainSec = nSecond;
	}
	else
	{
		//�����ð��� �ʷ� ����Ѵ�
		nSecond = static_cast<int>(ceil((double)nRemainTime));
		st_TransferInfo_Out.nRemainSec = nSecond;
	}
}

int CSimpleFtp::CreateRemoteDir2(CString strRemoteDir)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
//			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return -1;
		}
	}

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));

	if(strRemoteDir.GetLength() == 0 || strRemoteDir.Find(_T("/")) < 0)
		return -1;//����

	
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//����ڰ� �Է��� ��������� �������� '\'�� ������
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength()-1);//'/'�� ����(''(����)���� ��ȯ)

	if(FtpCreateDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)//���丮 ������ �õ�
	{
		//���丮 ���� ����
//		ShowLastErrorMsg();
		return 0;
	}

	if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//������ ����
		return FALSE;

	return 1;
}
//FTP������ ���丮�� ����
//�������丮�� �������� �ʴ°�� �������丮 ���� ����
int CSimpleFtp::CreateRemoteDir(CString strRemoteDir)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return -1;
		}
	}

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));

	if(strRemoteDir.GetLength() == 0 || strRemoteDir.Find(_T("/")) < 0)
		return -1;//����

	
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//����ڰ� �Է��� ��������� �������� '\'�� ������
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength()-1);//'/'�� ����(''(����)���� ��ȯ)

	if(FindRemoteDir(strRemoteDir) == TRUE)//�̹� ������ �����ϸ�
		return 1;

	//strDirPath ���ڿ��� �߰��� �ִ� ���� ����
	//���� /123/456/789 ��� /123, /123/456 ���� ����
	for(int i = 0; i < strRemoteDir.GetLength(); i++)
	{
		if(strRemoteDir.GetAt(i) == _T('/'))
		{
			CString strCreateDir = strRemoteDir.Left(i);
			if(FindRemoteDir(strCreateDir) != 0)//�˻����� �Ǵ� �̹� ���丮�� �����ϸ�
				continue;

			if(FtpCreateDirectoryA(hFtpConn, GetSendString(strCreateDir)) == FALSE)//���丮 ������ �õ�
			{
				//���丮 ���� ����
				ShowLastErrorMsg();
				return 0;
			}

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//������ ����
				return FALSE;
		}
	}

	if(FtpCreateDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)//���� ���丮(/123/456/789) ����
	{
		ShowLastErrorMsg();
		return 0;
	}

	return 1;
}

//FTP������ ���丮�� �ִ��� �˻�
//�����ϸ� 1, �������� ������ 0, ���� -1
int CSimpleFtp::FindRemoteDir(CString strRemoteDir)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("������ ������ �� �����ϴ�"));
			return FALSE;
		}
	}

	if(strRemoteDir.GetLength() == 0 )
		return  -1;
	
	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));

	if(strRemoteDir == _T("/"))
		return 1;
	
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//���丮����� �������� '/'�� ������
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength()-1);//'/'�� ����(''(����)���� ��ȯ)

	FILE_LIST file_list;
	FILE_LIST::iterator itr;
	CString strParentDir = GetParentDirFromPath(strRemoteDir);//strRemoteDir�� '/123/456/789'��� '/123/456' ����
	if(GetRemoteFileList(strParentDir, file_list) == FALSE)//���ϸ��(���丮+����)�� ����
		return 0;
	itr = file_list.find(strRemoteDir);
	if(itr != file_list.end() && itr->second.isDir == TRUE)
		return 1; //ã����

	return 0;
}

//FTP������ ������ �����ϴ��� �˻�
//�����ϸ� 1, �������� ������ 0, ���� -1
int CSimpleFtp::FindRemoteFile(CString strRemoteFilePath)
{
	if(strRemoteFilePath.GetLength() == 0)
		return  -1;

	strRemoteFilePath.Replace(_T("\\"), _T("/"));
	strRemoteFilePath.Replace(_T("//"), _T("/"));

	if(strRemoteFilePath.GetAt(0) != _T('/'))
		strRemoteFilePath = _T("/") + strRemoteFilePath;

	FILE_LIST file_list;
	FILE_LIST::iterator itr;
	CString strParentDir = GetParentDirFromPath(strRemoteFilePath);//strRemoteFilePath�� '/123/456/789.txt'��� '/123/456' ����
	GetRemoteFileList(strParentDir, file_list);//���ϸ��(���丮+����)�� ����
	itr = file_list.find(strRemoteFilePath);
	if(itr != file_list.end() && itr->second.isDir == FALSE)
		return 1; //ã����

	return 0;//��ã����
}

//���� �Ǵ� ���丮 �̸�����
BOOL CSimpleFtp::RenameRemoteFile(CString strRemoteOldPath, CString strRemoteNewPath)
{

	if(FtpRenameFileA(hFtpConn, GetSendString(strRemoteOldPath), GetSendString(strRemoteNewPath)) == FALSE)
	{
		ShowLastErrorMsg();
		return FALSE;
	}
	
	return TRUE;
}

//FTP������ �������丮 ���� ����
BOOL CSimpleFtp::DeleteRemoteDir(CString strRemoteDir)
{

	if(strRemoteDir.GetLength() == 0)
		return FALSE;
	strRemoteDir.Replace(_T("\\"), _T("/"));

	if(strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//����ڰ� �Է��� ��������� �������� '\'�Ǵ� '/'�� ������
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1); //����
	if(strRemoteDir.GetAt(0) != _T('/'))
		strRemoteDir = _T("/") + strRemoteDir;

	FILE_LIST del_list;
	FILE_LIST::reverse_iterator itr;//���� �ݺ���

	//��������� ����
	if(GetRemoteFileListWithSubDir(strRemoteDir, del_list) == FALSE)
		return FALSE;	//������� ���ϱ� ����

	
	//del_list�� ����� �������� ����(�������丮���� �����ؾ� �ϹǷ�)
	for(itr = del_list.rbegin(); itr != del_list.rend(); itr++)
	{
		if(itr->second.isDir)//���丮�̸�
		{
			//���丮 ����
			if(FtpRemoveDirectoryA(hFtpConn, GetSendString(itr->second.szFilePath)) == FALSE)
			{
				ShowLastErrorMsg();
				return FALSE;
			}

		}
		else//�����̸�
		{
			if(DeleteRemoteFile(itr->second.szFilePath) == FALSE)//���� ����
				return FALSE;
		}
	}

	if(FtpSetCurrentDirectoryA(hFtpConn, GetSendString(GetParentDirFromPath(strRemoteDir))) == FALSE)//��FTP ������ ���丮�� �̵��ϰ� ����Ʈ�� ���ؾ� �Ѵ�
	{
		ShowLastErrorMsg();
		return FALSE;//���丮 �̵� ����
	}

	//���� ���� ���丮 ����
	if(FtpRemoveDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)
	{
		ShowLastErrorMsg();
		return FALSE;
	}

	return TRUE;
}

//FTP������ ���� ����
BOOL CSimpleFtp::DeleteRemoteFile(CString strRemoteFilePath)
{
	if(strRemoteFilePath.GetLength() == 0)
		return FALSE;
	strRemoteFilePath.Replace(_T("\\"), _T("/"));

	if(strRemoteFilePath.GetAt(strRemoteFilePath.GetLength()-1) == _T('/'))//����ڰ� �Է��� ��������� �������� '\'�Ǵ� '/'�� ������
		strRemoteFilePath = strRemoteFilePath.Left(strRemoteFilePath.GetLength() - 1); //����

	if(strRemoteFilePath.GetAt(0) != _T('/'))
		strRemoteFilePath = _T("/") + strRemoteFilePath;

	
	if(FtpSetCurrentDirectoryA(hFtpConn, GetSendString(GetParentDirFromPath(strRemoteFilePath))) == FALSE)//��FTP ������ ���丮�� �̵��ϰ� ����Ʈ�� ���ؾ� �Ѵ�
	{
		ShowLastErrorMsg();
		return FALSE;//���丮 �̵� ����
	}

	if(FtpDeleteFileA(hFtpConn, GetSendString(strRemoteFilePath)) == FALSE)
	{
		ShowLastErrorMsg();
		return FALSE;
	}

	return TRUE;
}

BOOL CSimpleFtp::SetRemoteDirectory(CString strRemoteDir)
{
	if(FtpSetCurrentDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)
	{
		ShowLastErrorMsg();
		return FALSE;
	}
	
	return TRUE;
};

BOOL CSimpleFtp::GetRemoteDirectory(__out CString& strRemoteDir)
{
	char szRemoteDir[1024]={0,};
	DWORD dwLen = 1023;
	if(FtpGetCurrentDirectoryA(hFtpConn, szRemoteDir, &dwLen) == FALSE)
	{
		ShowLastErrorMsg();
		return FALSE;
	}

#ifdef UNICODE
	if(bUtf8)
		strRemoteDir = CA2W(szRemoteDir, CP_UTF8);
	else
		strRemoteDir = CA2W(szRemoteDir);
#else
	CStringW strRemoteDirW = CA2W(szRemoteDir, CP_UTF8);
	if(bUtf8)
		strRemoteDir = CW2A(strRemoteDirW);
	else
		strRemoteDir = szRemoteDir;
#endif
	
	if(strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//����ڰ� �Է��� ��������� �������� '/'�� ������
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1); //����

	
	return TRUE;
}

//FTP �����߻��� �����޽����� ������
void CSimpleFtp::ShowLastErrorMsg()
{
	TCHAR szErrorMsg[1024]={0,};
	DWORD nBuffSize = 1023, nErrorCode = 0;
	InternetGetLastResponseInfo(&nErrorCode, szErrorMsg, &nBuffSize);
	if(_tcslen(szErrorMsg) > 0)
		AfxMessageBox(szErrorMsg);
}

//FTP������ ���� utf8�� �����ϴ� ������ �ְ� �������� �ʴ� ������ ����
//Login�Ҷ� ���ڰ����� bUtf8�� ������ �°� �����ϸ� �ڵ����� ������
CStringA CSimpleFtp::GetSendString(CString strSend)
{
	CStringA strSendA;
	CStringW strSendW;

#ifdef UNICODE
	if(bUtf8)
		strSendA = CW2A(strSend, CP_UTF8);//���ڿ��� UTF8�� ���ڵ�
	else
		strSendA = CW2A(strSend);
#else
	strSendW = CA2W(strSend);
	if(bUtf8)
		strSendA = CW2A(strSendW,  CP_UTF8);//���ڿ��� UTF8�� ���ڵ�
	else
		strSendA = strSend;
#endif
	
	return strSendA;
}

//c:\123\456.txt��� 456.txt�� ����
CString CSimpleFtp::GetFileNameFromPath(CString strFilePath)
{
	if(strFilePath.GetLength() == 0)
		return _T("");

	CString strFileName;
	TCHAR szSeperator = _T('\\');

	if(strFilePath.Find(_T('\\')) >= 0)
		szSeperator = _T('\\');
	else
		szSeperator = _T('/');

	if(strFilePath.GetAt(strFilePath.GetLength()-1) == szSeperator)//����ڰ� �Է��� ��������� �������� '\'�Ǵ� '/'�� ������
		strFilePath = strFilePath.Left(strFilePath.GetLength() - 1); //����

	strFilePath.MakeReverse();
	int nPos = strFilePath.Find(szSeperator);
	strFilePath.MakeReverse();
	if(nPos > 0)
	{
		strFileName = strFilePath.Right(nPos);
	}
	else 
		strFileName = strFilePath;

	return strFileName;

}

//c:\123\456\789.txt��� c:\123\456�� ����
CString CSimpleFtp::GetParentDirFromPath(CString strFilePath)
{
	if(strFilePath.GetLength() == 0)
		return _T("");

	CString strDir;
	TCHAR szSeperator = _T('\\');
	
	if(strFilePath.Find(_T('\\')) >= 0)
		szSeperator = _T('\\');
	else
		szSeperator = _T('/');

	if(strFilePath.GetAt(strFilePath.GetLength()-1) == szSeperator)//����ڰ� �Է��� ��������� �������� '\'�Ǵ� '/'�� ������
		strFilePath = strFilePath.Left(strFilePath.GetLength() - 1); //����


	strFilePath.MakeReverse();
	int nPos = strFilePath.Find(szSeperator);
	strFilePath.MakeReverse();
	
	if(nPos > 0)
	{
		strDir = strFilePath.Left(strFilePath.GetLength() - nPos - 1);
		if(strDir.GetLength() == 0 && szSeperator == _T('/'))
			strDir = _T("/");
	}
	else
		strDir = _T("");

	return strDir;
}

//�̾�ø��� �뵵�� ������ ������ ����
BOOL CSimpleFtp::ReadyUploadResume(CString strRemoteFilePath, UINT64 nStartFilePointer)
{
	CString strCommand;
	
	FtpSetCurrentDirectoryA(hFtpConn, GetSendString(GetParentDirFromPath(strRemoteFilePath)));

	strCommand.Format(_T("REST %I64d"), nStartFilePointer);
	if(FtpCommandA(hFtpConn, FALSE, FTP_TRANSFER_TYPE_BINARY, GetSendString(strCommand), 0, 0) == FALSE)
		return FALSE;//�̾�ø��� ���� ����
	if(IsPositiveResponse() == FALSE)
		return FALSE;
	
	return TRUE;
}

//�̾�ø��� �뵵�� ������ ������ ����
//APPE�� ����ϸ� ������ APPE�� �̿��Ϸ��� PORT ��ɾ ���� ����ؾ� �ϴ� �� �ϴ�
//������ PORT ��ɾ ����Ϸ��� ���� FTP ���� ������ ���� port�� �˾ƾ� �ϴµ� �װ� �ȵȴ�
//MSDN���� ���� ���� �����ϸ� �ݹ��Լ��� SOCKADDR �����Ͱ� �Ѿ�´ٰ� �Ǿ��ִµ� �ȳѾ���� �� ����..��.��
BOOL CSimpleFtp::ReadyDownloadResume(UINT64 nStartFilePointer)
{
	CString strCommand;
	
	strCommand.Format(_T("REST %I64d"), nStartFilePointer);
	if(FtpCommandA(hFtpConn, FALSE, FTP_TRANSFER_TYPE_BINARY, GetSendString(strCommand), 0, 0) == FALSE)
		return FALSE;
	if(IsPositiveResponse() == FALSE)
		return FALSE;
	
	return TRUE;
}


//��ɿ� ���� ������ ������ ��������, ����������?
BOOL CSimpleFtp::IsPositiveResponse()
{
	TCHAR szReply[1024]={0,};
	DWORD nBuffSize = 1023, nErrorCode = 0;
	BOOL bOk = FALSE;
	InternetGetLastResponseInfo(&nErrorCode, szReply, &nBuffSize);
	
	if(_tcslen(szReply) == 0)
		return FALSE;
	
	if(szReply[0] == _T('1') || szReply[0] == _T('2') || szReply[0] == _T('3'))
		return TRUE;

	return FALSE;
}


//���ͳ� ���¸� �˷��ִ� �ݹ��Լ�
//���� dwContext�� CSimpleFtp Ŭ���������͸� �������� ���·� �����ߴ�
void __stdcall CallMaster(HINTERNET hInternet,  DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
	CSimpleFtp* pSimpleFtp;
	
	switch (dwInternetStatus)
	{
	case INTERNET_STATUS_CLOSING_CONNECTION:
		break;
	case INTERNET_STATUS_CONNECTED_TO_SERVER:
		break;
	case INTERNET_STATUS_CONNECTING_TO_SERVER:
		break;
	case INTERNET_STATUS_CONNECTION_CLOSED://������ ������ ����
		pSimpleFtp = (CSimpleFtp*)dwContext;
		pSimpleFtp->LogOut();//�α׾ƿ� ó��
		break;
	case INTERNET_STATUS_HANDLE_CLOSING:
		break;
	case INTERNET_STATUS_HANDLE_CREATED:
		break;
	case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
		break;
	case INTERNET_STATUS_NAME_RESOLVED:
		break;
	case INTERNET_STATUS_RECEIVING_RESPONSE:
		break;
	case INTERNET_STATUS_RESPONSE_RECEIVED:		
		break;
	case INTERNET_STATUS_REDIRECT:
		break;
	case INTERNET_STATUS_REQUEST_COMPLETE:
		break;
	case INTERNET_STATUS_REQUEST_SENT:
		break;
	case INTERNET_STATUS_RESOLVING_NAME:
		break;
	case INTERNET_STATUS_SENDING_REQUEST:
		break;
	case INTERNET_STATUS_STATE_CHANGE:
		break;
	default:
		break;
	}
}

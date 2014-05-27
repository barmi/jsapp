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

//로그인
BOOL CSimpleFtp::Login(CString strIP, CString strID, CString strPasswd, UINT nPort, HWND hCallWnd, BOOL bUtf8, BOOL bPassive)
{
	if(IsConnected() == TRUE)//FTP 서버에 기 연결되어있으면
		LogOut();//로그아웃
	

	this->strIP = strIP; this->strID = strID; this->strPasswd = strPasswd; this->nPort = nPort;
	this->hCallWnd = hCallWnd;//전송중인 정보(전송데이터크기, 남은시간, 속도)를 받을 윈도우 핸들(메시지 전송에 이용)
	this->bUtf8 = bUtf8;//FTP 서버가 UTF8을 지원하는 경우
	this->bPassive = bPassive;

	hSession = InternetOpen(NULL,INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);//세션 생성
	if(hSession == NULL) 
		return FALSE;
	
	INTERNET_STATUS_CALLBACK iscCallback;// Holder for the callback
	iscCallback = InternetSetStatusCallback(hSession, (INTERNET_STATUS_CALLBACK)CallMaster);//CallMaster를 콜백함수 설정

	if(bPassive == TRUE)//패시브 모드이면
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , (DWORD_PTR)this);//패시브 모드
	else//패시브 모드가 아니면
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, 0, (DWORD_PTR)this);//Active 모드
	
	if(hFtpConn == NULL)
	{
		//FTP연결에 실패하면
		TRACE(_T("접속실패"));
		return FALSE;
	}

	return TRUE;//FTP서버 접속 성공
}

//시간지연으로 연결이 끊긴경우 사용자가 입력한 정보를 가지고 다시 로그인시킨다
BOOL CSimpleFtp::Login()
{
	if(IsConnected() == TRUE)//FTP 서버에 기 연결되어있으면
		return TRUE;

	if(strIP.GetLength() == 0 || strID.GetLength() == 0 || strPasswd.GetLength() == 0 || nPort == 0)
		return FALSE;

	hSession = InternetOpen(NULL,INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);//세션 생성
	if(hSession == NULL) 
		return FALSE;

	INTERNET_STATUS_CALLBACK iscCallback;// Holder for the callback
	iscCallback = InternetSetStatusCallback(hSession, (INTERNET_STATUS_CALLBACK)CallMaster);//CallMaster를 콜백함수 설정

	if(bPassive == TRUE)//패시브 모드이면
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, INTERNET_FLAG_PASSIVE , (DWORD_PTR)this);//패시브 모드
	else//패시브 모드가 아니면
		hFtpConn= InternetConnect(hSession, strIP, nPort, strID, strPasswd, INTERNET_SERVICE_FTP, 0, (DWORD_PTR)this);//Active 모드

	if(hFtpConn == NULL)
	{
		//FTP연결에 실패하면
		InternetCloseHandle(hSession);
		AfxMessageBox(_T("FTP서버 접속실패"));
		return FALSE;
	}

	return TRUE;//FTP서버 접속 성공

}

//로그 아웃
void CSimpleFtp::LogOut()
{
	if(hCloseEvent)
	{
		SetEvent(hCloseEvent);//전송중인 데이터가 있다면 강제 종료
		CloseHandle(hCloseEvent);
		hCloseEvent = NULL;
	}

	if(hFtpConn)
	{
		InternetCloseHandle(hFtpConn);//FTP 핸들 종료
		hFtpConn = NULL;
	}
	if(hSession)
	{
		InternetCloseHandle(hSession);//세션 종료
		hSession = NULL;
	}
	
	hCallWnd = NULL;
}

//FTP서버에 연결되어있으면 TRUE, 아니면 FALSE
BOOL CSimpleFtp::IsConnected()
{
	DWORD dwState = 0;
	
	if(hFtpConn == NULL || hSession == NULL)//세션핸들이 NULL인 경우
		return FALSE;//연결안됨

	return TRUE;
}

//strRemoteFilePath을 strLocalFilePath 경로에 저장(파일만 다운로드)
BOOL CSimpleFtp::DownloadFile(CString strRemoteFilePath, CString strLocalFilePath, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return FALSE;
		}
	}
	TransferInfo_In st_TransferInfo_In;//전송기본정보(전송중인 파일명, 전송시작시간, 전송현재시간 전송할 데이터의 총 크기 현재 전송된 데이터 크기
	TransferInfo_Out st_TransferInfo_Out;//전송기본정보를 이용하여 전송속도, 남은시간 등의 추가정보를 구함

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));


	st_TransferInfo_In.dwStartTime = ::GetTickCount();//전송시작시간
	_tcscpy_s(st_TransferInfo_In.szFilePath, strLocalFilePath);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	FILE* fp = NULL;
	fopen_s(&fp, CT2A(strLocalFilePath), bResume ? "ab" : "wb");//이어받기 또는 일반 다운로드 파일생성
	if(!fp)
	{
		TRACE(_T("파일 열기 실패"));
		return FALSE;
	}

	if(GetRemoteFileSize(strRemoteFilePath, st_TransferInfo_In.nTotFileSize) == FALSE)//파일의 크기를 구함
		return FALSE;

	if(bResume)
	{
		_fseeki64(fp, 0, SEEK_END);
		UINT64 nFileSize = _ftelli64(fp);//로컬파일크기

		st_TransferInfo_In.nTransferSize = nFileSize;//로컬 파일크기
		st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
		if(hCallWnd != NULL)//Login에서 hCallWnd가 설정되면
		{
			//현재 다운로드 정보를 보낸다
			GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
			::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);	
			PUMP_MESSAGES();
		}

		if((st_TransferInfo_In.nTotFileSize - nFileSize) <=0 )//로컬 파일크기가 FTP서버의 파일보다 크다면
		{
			fclose(fp);
			return TRUE;//다운로드 받을 필요 없음
		}

		if(ReadyDownloadResume(nFileSize) == FALSE)//이어받기 모드 준비
		{
			//준비 실패
			fclose(fp);
			fopen_s(&fp, CT2A(strLocalFilePath), "wb");//다운로드 파일생성
			if(!fp)
				return FALSE;
		}
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//FTP서버의 파일 오픈
	HINTERNET hFile = FtpOpenFileA(hFtpConn, GetSendString(strRemoteFilePath), GENERIC_READ, INTERNET_FLAG_TRANSFER_BINARY, 0);
	if(hFile == NULL)
		return FALSE;//실패
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//다운로드 받을디렉토리가 없으면 생성하고 파일을 저장
	CString strDownloadDir = GetParentDirFromPath(strLocalFilePath);
	if(strDownloadDir.GetLength() > 0)
	{
		//경로를 포함한 파일명인 경우
		if(CreateLocalDir(strDownloadDir) <= 0)//다운로드 받을 디렉토리 생성
			return FALSE;//디렉토리 생성 실패
	}
	else
	{
		//경로 없이 파일명만 있는 경우
		strDownloadDir = GetWorkingDir();
		strLocalFilePath = strDownloadDir + _T("\\") + strLocalFilePath;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//파일 다운로드
	BYTE Buff[MAX_BUFFER_SIZE]={0,};
	DWORD dwRead = 0;
	UINT64 nTotRead = st_TransferInfo_In.nTransferSize;
	do
	{
		if(InternetReadFile(hFile, Buff, MAX_BUFFER_SIZE, &dwRead) == FALSE)//파일데이터를 읽어옴
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

		if(hCallWnd != NULL)//Login에서 hCallWnd가 설정되면
		{
			//현재 다운로드 정보를 보낸다
			GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
			::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);	
			PUMP_MESSAGES();
		}

		if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
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


//하위디렉토리 포함하여 다운로드한다
//FTP서버에 "/123/456/789" 라는 디렉토리가 존재할때
//strRemoteDir = "/123": , strLocalDir = "c:\SimpleFtp" 라면
//최종 로컬디스크에 생성된 모습은 c:\SimpleFtp\456\789 임
BOOL CSimpleFtp::DownloadWithSubDir(CString strRemoteDir, CString strLocalDir, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return FALSE;
		}
	}

	FILE_LIST down_list;
	FILE_LIST::iterator itr;
	
	if(GetRemoteFileListWithSubDir(strRemoteDir, down_list) == FALSE)//하위디렉토리 포함하여 다운로드목록을 down_list에 저장
		return FALSE;
	
	//로컬경로명의 마지막에 '\'가 있으면 제거한다
	if(strLocalDir.GetAt(strLocalDir.GetLength()-1) == _T('\\'))//사용자가 입력한 폴더경로의 마지막에 '\'가 있으면
		strLocalDir = strLocalDir.Left(strLocalDir.GetLength()-1);//'\'를 ''(공백)으로 변환

	UINT64 nTotRead = 0;//전체 다운로드 데이터 크기
	TransferInfo_In st_TransferInfo_In;//전송기본정보(전송중인 파일명, 전송시작시간, 전송현재시간 전송할 데이터의 총 크기 현재 전송된 데이터 크기
	TransferInfo_Out st_TransferInfo_Out;//전송기본정보를 이용하여 전송속도, 남은시간 등의 추가정보를 구함

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));

	st_TransferInfo_In.dwStartTime = ::GetTickCount();
	st_TransferInfo_In.nTotFileSize = GetTotFileSize(down_list);//다운로드 받을 데이터의 총크기를 구함

	//다운로드 목록을 순회하면서 파일다운로드
	for(itr = down_list.begin(); itr != down_list.end(); itr++)
	{
		if(!IsConnected())//서버와 연결이 되어 있지 않다면
			return FALSE;//다운로드 종료

		CString strRemotePath = itr->second.szFilePath;
		CString strLocalFilePath;

		if(strRemoteDir != _T("/"))
			strRemotePath.Replace(strRemoteDir, _T(""));//사용자가 지정한 원격디렉토리의 하위디렉토리를 다운로드

		strRemotePath.Replace(_T("/"), _T("\\"));
		strLocalFilePath = strLocalDir + _T("\\") + strRemotePath;//다운로드에 필요한 로컬에 만들어야하는 디렉토리
		strLocalFilePath.Replace(_T("\\\\"), _T("\\"));

		if(itr->second.isDir == TRUE)//디렉토리인지?
		{
			if(CreateLocalDir(strLocalFilePath) == FALSE)//디렉토리 생성(상위디렉토리가 없다면 강제로 생성)
			{
				TRACE(_T("로컬폴더 생성 실패"));
				return FALSE;
			}
			continue;
		}
		else
		{
			if(CreateLocalDir(GetParentDirFromPath(strLocalFilePath)) == FALSE)//디렉토리 생성(상위디렉토리가 없다면 강제로 생성)
			{
				TRACE(_T("로컬폴더 생성 실패"));
				return FALSE;
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		FILE* fp = NULL;
		fopen_s(&fp, CT2A(strLocalFilePath), bResume ? "ab" : "wb");//이어받기 또는 일반 다운로드 파일생성
		if(!fp)
		{
			TRACE(_T("파일 열기 실패"));
			return FALSE;
		}
		
		if(bResume)//이어받기?
		{
			//이어받기라면
			INT64 nReadOffset = 0;
			fseek(fp, 0, SEEK_END);
			UINT64 nFileSize = _ftelli64(fp);
			if(ReadyDownloadResume(nFileSize) == FALSE)//이어받기 모드 준비
			{
				//이어받기 준비 실패(서버지원 안함)
				fclose(fp);
				fopen_s(&fp, CT2A(strLocalFilePath), "wb");//처음부터 다시 다운로드
				if(!fp)
					return FALSE;
				
				bResume = FALSE;
			}
			else
			{
				nTotRead += nFileSize;
				st_TransferInfo_In.nTransferSize = nTotRead;
				st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
				nReadOffset = itr->second.nFileSize - nFileSize;//FTP서버 파일크기 - 로컬파일크기
				if(nReadOffset <= 0)
				{
					fclose(fp);
					if(hCallWnd != NULL)
					{
						GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
						::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);//다운로드 정보를 부모윈도로 보낸다
						PUMP_MESSAGES();
					}
					continue;
				}
			}
			
		}
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		_tcscpy_s(st_TransferInfo_In.szFilePath, strLocalFilePath);
		//FTP파일 오픈
		HINTERNET hFile = FtpOpenFileA(hFtpConn, GetSendString(itr->second.szFilePath), GENERIC_READ, FTP_TRANSFER_TYPE_BINARY | INTERNET_FLAG_TRANSFER_BINARY, 0);
		if(hFile == FALSE)
			return FALSE;

		
		BYTE Buff[MAX_BUFFER_SIZE]={0,};
		DWORD dwRead = 0;
		do
		{
			if(InternetReadFile(hFile, Buff, MAX_BUFFER_SIZE, &dwRead) == FALSE)//서버로부터 데이터를 읽는다
			{
				fclose(fp);
				InternetCloseHandle(hFile);
				ShowLastErrorMsg();
				return FALSE;
			}
			fwrite(Buff, dwRead, 1, fp);

			nTotRead += dwRead;
			st_TransferInfo_In.nTransferSize = nTotRead;//현재까지 총 다운로드한 데이터의 크기
			st_TransferInfo_In.dwCurrentTime = ::GetTickCount();//지금가지 진행중인 시간

			if(hCallWnd != NULL)
			{
				GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
				::SendMessage(hCallWnd, WM_SHOW_DOWNLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);//다운로드 정보를 부모윈도로 보낸다
				PUMP_MESSAGES();
			}

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
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


//파일을 업로드한다
BOOL CSimpleFtp::UploadFile(CString strLocalFilePath, CString strRemoteFilePath, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return FALSE;
		}
	}

	int nPos = strLocalFilePath.Find(_T("\\"));
	if(nPos < 0)//'\'를 못 찾으면
		strLocalFilePath = GetWorkingDir() + _T("\\") + strLocalFilePath;//경로없이 파일명만 있는 경우 실행파일이 있는 경로를 디폴트 경로로 한다

	if(strRemoteFilePath != _T("/"))
	{
		CString strRemoteDir = GetParentDirFromPath(strRemoteFilePath);

		strRemoteDir.Replace(_T("\\"), _T("/"));
		strRemoteDir.Replace(_T("//"), _T("/"));

		if(CreateRemoteDir(strRemoteDir) <= 0)
		{
			AfxMessageBox(_T("디렉토리 생성 실패"));
			return FALSE;
		}
	}

	
	HINTERNET hFile = NULL;
	UINT64 nStartFilePointer = 0;
	if(bResume)
	{
		if(GetRemoteFileSize(strRemoteFilePath, nStartFilePointer) == FALSE)//파일크기 구하기
			nStartFilePointer = 0;

		if(nStartFilePointer > 0)//서버의 파일이 0보다 크다면
			ReadyUploadResume(strRemoteFilePath, nStartFilePointer);//이어 올리기 준비
		else
			bResume = FALSE;//이어올리기 못함
	}
	
	TransferInfo_In st_TransferInfo_In;//전송기본정보(전송중인 파일명, 전송시작시간, 전송현재시간 전송할 데이터의 총 크기 현재 전송된 데이터 크기
	TransferInfo_Out st_TransferInfo_Out;//전송기본정보를 이용하여 전송속도, 남은시간 등의 추가정보를 구함

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));

	FILE* fp = NULL;
	fopen_s(&fp, CT2A(strLocalFilePath), "rb");
	if(!fp)
	{
		InternetCloseHandle(hFile);
		TRACE(_T("파일 열기 실패"));
		return FALSE;
	}

	_fseeki64(fp, 0, SEEK_END);
	st_TransferInfo_In.nTotFileSize = _ftelli64(fp);//업로드할 파일의 크기를 구함
	st_TransferInfo_In.dwStartTime = ::GetTickCount();//전송시작 시간
	if(bResume)
	{
		_fseeki64(fp, nStartFilePointer, SEEK_SET);//파일 포인터를 이어올릴 지점으로 이동
	}
	else
		_fseeki64(fp, 0, SEEK_SET);//파일 포인터를 시작점으로 이동

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
		
		if((st_TransferInfo_In.nTotFileSize - nStartFilePointer) <= 0)//서버의 파일이 업로드할 로컬파일보다 크다면
			return TRUE;//파일전송완료(파일전송할 필요없음)
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
	while(!feof(fp))//로컬파일을 다 읽을때까지 반복
	{
		_tcscpy_s(st_TransferInfo_In.szFilePath, strLocalFilePath);//전송중인 파일명(경로포함)
		size_t nReadSize = fread(Buff, 1, MAX_BUFFER_SIZE, fp);

		if(nReadSize == 0)
			break;

		UINT dwSend = 0;
		do
		{	
			//FTP서버에 업로드
			if(InternetWriteFile(hFile, Buff+dwSend, nReadSize-dwSend, &dwWrite) == FALSE)
			{
				InternetCloseHandle(hFile);
				fclose(fp);
				ShowLastErrorMsg();
				return FALSE;
			}
			nTotSend += dwWrite;//총 전송된 크기
			dwSend += dwWrite;//현재 파일의 전송된 크기
			*Buff += dwSend;//MAX_BUFFER_SIZE

			st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
			st_TransferInfo_In.nTransferSize = nTotSend;

			if(hCallWnd)
			{
				GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
				::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
				PUMP_MESSAGES();
			}

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
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

//하위디렉토리 포함하여 업로드
BOOL CSimpleFtp::UploadWithSubDir(CString strLocalDir, CString strRemoteDir, BOOL bResume)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return FALSE;
		}
	}

	if(strLocalDir.GetLength() == 0)
		return FALSE;

	if(strLocalDir.GetAt(strLocalDir.GetLength()-1) == _T('\\'))//폴더경로의 마지막이 '/'이면
		strLocalDir = strLocalDir.Left(strLocalDir.GetLength() - 1);//마지막 '/' 제거


	if(strRemoteDir.GetLength() == 0)
		return FALSE;

	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//폴더경로의 마지막이 '/'이면
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1);//마지막 '/' 제거

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));


	FILE_LIST upload_list;
	FILE_LIST::iterator itr;
	TransferInfo_In st_TransferInfo_In;//전송기본정보(전송중인 파일명, 전송시작시간, 전송현재시간 전송할 데이터의 총 크기 현재 전송된 데이터 크기
	TransferInfo_Out st_TransferInfo_Out;//전송기본정보를 이용하여 전송속도, 남은시간 등의 추가정보를 구함
	UINT64 nTotSend = 0;

	memset(&st_TransferInfo_In, 0, sizeof(TransferInfo_In));
	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));

	GetLocalFileListWithSubDir(strLocalDir, upload_list);
	st_TransferInfo_In.nTotFileSize = GetTotFileSize(upload_list);
	st_TransferInfo_In.dwStartTime = ::GetTickCount();

	//파일목록을 순회하며 업로드
	for(itr = upload_list.begin(); itr != upload_list.end(); itr++)
	{
		if(!IsConnected())//업로드 중에 서버와 연결이 끊겼다면
			return FALSE;//업로드 종료

		CString strSubDir, strLocalFilePath, strRemoteFilePath;
		_tcscpy_s(st_TransferInfo_In.szFilePath, itr->second.szFilePath);//전송중인 파일명(경로포함)

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
			strMsg.Format(_T("%s 디렉토리 생성 실패"), strCreateDir);
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
			//이어올리기 모드라면
			INT64 nWriteOffset = 0;
			if(GetRemoteFileSize(strRemoteFilePath, nStartFilePointer) == FALSE)//파일크기 구하기
				nStartFilePointer = 0;//FTP서버에 파일이 없기때문임(권한 문제일 수도 있음)

			
			nWriteOffset = itr->second.nFileSize - nStartFilePointer;//로컬파일크기 - FTP서버 파일크기
			if(nWriteOffset <= 0)//서버의 파일이 로컬파일보다 크다면
			{
				st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
				nTotSend += nStartFilePointer;//이어받기 이므로 이미 전송한 데이터로 계산
				st_TransferInfo_In.nTransferSize = nTotSend;
				if(hCallWnd)
				{
					GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
					::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
					PUMP_MESSAGES();
				}
				continue;//다음파일로
			}

			if(nStartFilePointer > 0)//서버의 파일이 0보다 크다면 
				ReadyUploadResume(strRemoteFilePath, nStartFilePointer);//이어올리기 준비
			
		}
		//서버의 파일을 오픈(이어올리기든 아니든 FTP서버의 파일은 오픈)
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
			TRACE(_T("파일 열기 실패"));
			return FALSE;
		}

		if(bResume)
		{
			_fseeki64(fp, nStartFilePointer, SEEK_SET);//파일 포인터를 이어올릴 지점으로 이동
			nTotSend += nStartFilePointer;//이어받기 이므로 이미 전송한 데이터로 계산
			st_TransferInfo_In.nTransferSize = nTotSend;
		}
		else
			_fseeki64(fp, 0, SEEK_SET);//파일 포인터를 시작점으로 이동

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
				//업로드, 못 보낸 데이터가 있다면 반복해서 보내려고 코드를 작성하긴 했으나
				//InternetWriteFile은 내부단에서 지정한 크기만큼 데이터를 항상 전송하는 듯 하다
				if(InternetWriteFile(hFile, Buff+dwSend, nReadSize-dwSend, &dwWrite) == FALSE)
				{
					InternetCloseHandle(hFile);
					fclose(fp);
					ShowLastErrorMsg();
					return FALSE;
				}

				nTotSend += dwWrite;//총 업로드 데이터 크기
				dwSend += dwWrite;//현재 파일의 업로드 데이터 크기, 

				st_TransferInfo_In.dwCurrentTime = ::GetTickCount();
				st_TransferInfo_In.nTransferSize = nTotSend;

				if(hCallWnd)
				{
					GetTransferInfo(st_TransferInfo_In, st_TransferInfo_Out);
					::SendMessage(hCallWnd, WM_SHOW_UPLOAD_STATE, 0, (LPARAM)&st_TransferInfo_Out);
					PUMP_MESSAGES();
				}

				if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
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


//하위디렉토리 포함한 FTP 서버의 파일목록을 구함
//FtpFileFind는 재귀호출을 사용할 수 없는 속성때문에 아래와 같이 작성
BOOL CSimpleFtp::GetRemoteFileListWithSubDir(__in CString strRemoteDir, __out FILE_LIST& file_list)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return FALSE;
		}
	}

	//현재 디렉토리에 들어있는 파일 및 디렉토리 목록을 구함(하위디렉토리 미포함)
	if(GetRemoteFileList(strRemoteDir, file_list) == FALSE)
		return FALSE;

	//하위디렉토리 포함하여 파일 및 디렉토리 목록을 구함
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



//파일 및 디렉토리 목록을 구함(하위디렉토리 미포함)
//성공 : TRUE, 실패 : FALSE
BOOL CSimpleFtp::GetRemoteFileList(__in CString strRemoteDir, __out FILE_LIST& file_list)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return FALSE;
		}
	}

	if(strRemoteDir.GetLength() == 0)
		return FALSE;

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//사용자가 입력한 폴더경로의 마지막에 '\'가 있으면
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1); //'/'를 제거


	WIN32_FIND_DATAA find_data;
	memset(&find_data, 0, sizeof(WIN32_FIND_DATAA));
	CString strAddPath;
	
	//파일질라 서버는 원격지경로명을 직접입력해도 파일목록을 구해오는 반면 허접한 알FTP는 현재경로의 파일목록만 전송한다
	if(FtpSetCurrentDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)//알FTP 서버는 디렉토리를 이동하고 리스트를 구해야 한다
		return FALSE;//디렉토리 이동 실패

	//파일 및 디렉토리 목록을 구함
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

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
			{
				InternetCloseHandle(hFind);
				return FALSE;
			}

		}while(bFind);
	}

	InternetCloseHandle(hFind);

	return TRUE;
}


//WIN32_FIND_DATAA의 정보를 file_list에 추가한다
//여기서 파일 및 디렉토리명의 문자셋 때문에 고민을 좀 했는데
//알FTP는 유니코드(UTF8)을 지원하지 않으나 파일질라(FileZilla)나 리눅스의 ProFtp는 유니코드만 지원하는 등
//FTP서버 프로그램에 따라 문자셋이 서로 다름. 따라서 두가지 경우를 모두 지원하기 위해
//Login할때 Utf8값을 설정하도록 하였음. 알FTP인 경우는 bUtf8 = false, 그 외 Utf8을 지원하는 서버는 bUtf8 = true로 설정
CString CSimpleFtp::AddRemoteFile(const CString strCurrentRemoteDir, const WIN32_FIND_DATAA& find_data, FILE_LIST& file_list)
{
	FileInfo st_FileInfo;
	memset(&st_FileInfo, 0, sizeof(FileInfo));
	CString strName;//디렉토리 또는 파일명

	//유니코드 컴파일 설정이라면
#ifdef UNICODE
	if(bUtf8 == TRUE)
		strName = CA2W(find_data.cFileName, CP_UTF8);//디렉토리 또는 파일명
	else
		strName = CA2W(find_data.cFileName);
#else//멀티바이트 컴파일 설정이라면
	if(bUtf8 == TRUE)
	{
		CStringW strTemp =  CA2W(find_data.cFileName, CP_UTF8);//디렉토리 또는 파일명
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
		if(strName.GetAt(strName.GetLength()-1) == _T('/'))//사용자가 입력한 폴더경로의 마지막에 '/'가 있으면
			strName = strName.Left(strName.GetLength() - 1);
		strName.Replace(_T("\\"), _T("/"));
		strName.Replace(_T("//"), _T("/"));
	}
	else
		st_FileInfo.isDir = FALSE;

	st_FileInfo.nFileSize = (find_data.nFileSizeHigh * (MAXDWORD + (__int64)1)) + find_data.nFileSizeLow;//파일크기
	FileTimeToSystemTime(&find_data.ftCreationTime, &st_FileInfo.stCreattionTime);//생성일(FTP에서 지원 안하는 듯)
	FileTimeToSystemTime(&find_data.ftLastAccessTime, &st_FileInfo.stLastAccessTime);//최종접근일(FTP에서 지원 안하는 듯)
	FileTimeToSystemTime(&find_data.ftLastWriteTime, &st_FileInfo.stLastWriteTime);//최종수정일(FTP지원)

	CString strRemoteFilePath = strCurrentRemoteDir + _T("/") + strName;
	if(strRemoteFilePath.GetAt(0) != _T('/'))
		strRemoteFilePath = _T("/") + strRemoteFilePath;
	strRemoteFilePath.Replace(_T("//"), _T("/"));
	_tcscpy_s(st_FileInfo.szFilePath, strRemoteFilePath);
	file_list.insert(pair<CString, FileInfo>(st_FileInfo.szFilePath, st_FileInfo));//맵 데이터에 추가

	return st_FileInfo.szFilePath;
}

//FTP서버의 파일크기를 구함
//알FTP에서는 FtpGetFileSize 값이 틀릴때가 있어 아래처럼 한다
BOOL CSimpleFtp::GetRemoteFileSize(__in CString strRemoteFilePath, __out UINT64& nRemoteFileSize)
{
	CString strRemoteDir = GetParentDirFromPath(strRemoteFilePath), strRemoteFileName = GetFileNameFromPath(strRemoteFilePath);
	nRemoteFileSize = 0;

	
	DWORD dwLow = 0, dwHigh = 0;

	FILE_LIST file_list;
	FILE_LIST::iterator itr;
	GetRemoteFileList(strRemoteDir, file_list);//파일목록을 구한다
	itr = file_list.find(strRemoteFilePath);
	if(itr != file_list.end() && itr->second.isDir == FALSE)
		nRemoteFileSize = itr->second.nFileSize;//파일크기
	else
		return FALSE;

	return TRUE;
}


//file_list에 저장된 목록의 총 파일크기를 구함
__out UINT64 CSimpleFtp::GetTotFileSize(__in FILE_LIST& file_list)
{
	unsigned __int64 nTotFileSize = 0;
	FILE_LIST::iterator itr;
	for(itr = file_list.begin(); itr != file_list.end(); itr++)
	{
		if(itr->second.isDir == FALSE) //파일인 경우
			nTotFileSize += itr->second.nFileSize;
	}

	return nTotFileSize;
}

//하위디렉토리 포함하여 디렉토리를 생성한다
//가령 strDirPath가 c:\123\456\789라면 c:\, c:\123, c:\123\456, c:\123\456\789 폴더를 생성한다
int CSimpleFtp::CreateLocalDir(CString strDirPath)
{
	strDirPath.Replace(_T("\\\\"), _T("\\"));
	if(strDirPath.GetLength() == 0)
		return -1;
	
	if(strDirPath.GetAt(strDirPath.GetLength()-1) == _T('\\'))//사용자가 입력한 폴더경로의 마지막에 '\'가 들어가 있으면
		strDirPath = strDirPath.Left(strDirPath.GetLength()-1);//'\'를 ''(공백)으로 변환


	BOOL bSuccess = TRUE;

	//strDirPath 문자열의 중간에 있는 폴더 생성
	//가령 c:\123\456\789 라면 c:\, c:\123, c:\123\456 폴더 생성
	for(int i = 0; i < strDirPath.GetLength(); i++)
	{
		if(strDirPath.GetAt(i) == _T('\\'))
		{
			CString strMakeDir = strDirPath.Left(i);
			if(_taccess_s(strMakeDir, 0) == ENOENT)//디렉토리가 존재하지 않으면
			{
				if(_tmkdir(strMakeDir) == ENOENT)//디렉토리 생성을 시도
				{
					//디렉토리 생성에 실패하면
					bSuccess = FALSE;
					return bSuccess;
				}
			}
		}
	}

	if(_tmkdir(strDirPath) == ENOENT)//최종 디렉토리(c:\123\456\789) 생성
		bSuccess = FALSE;//실패

	return bSuccess;
}

//특정 디렉토리 파일목록 구하기
UINT64 CSimpleFtp::GetLocalFileListWithSubDir(CString strDirPath, FILE_LIST& file_list) 
{
	static UINT64 m_TotFileSize = 0;//재귀함수로 목록을 구하기 때문에 static으로 해야 이전 값을 유지할 수 있다
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

			CString strFileName = m_FileFind.GetFileName(); //서버에 있는 경로
			CString strFullPath = strDirPath + _T("\\") + strFileName;


			// '.', '..'은 제외
			if(m_FileFind.IsDots())
				continue;

			CTime m_Time;
			m_FileFind.GetCreationTime(m_Time);
			m_Time.GetAsSystemTime(st_FileInfo.stCreattionTime);//생성일
			m_FileFind.GetLastWriteTime(m_Time);
			m_Time.GetAsSystemTime(st_FileInfo.stLastWriteTime);//최종 수정일
			m_FileFind.GetLastAccessTime(m_Time);
			m_Time.GetAsSystemTime(st_FileInfo.stLastAccessTime);//최종 접근일

			//파일 전체경로
			CString strSavePath = strFullPath;
			////////////////////////////////////////////////////////////
			if(m_FileFind.IsDirectory())      // 찾은 파일이 디렉토리이면
			{
				if(strSavePath.GetLength() > 0)
					_tcscpy_s(st_FileInfo.szFilePath, strSavePath);

				st_FileInfo.isDir = TRUE;

				// 하위 디렉토리 이므로 재귀 호출하여 하위 디렉토리로 이동
				GetLocalFileListWithSubDir(strDirPath + _T("\\") + strFileName, file_list);
			}
			else
			{
				st_FileInfo.nFileSize = m_FileFind.GetLength();
				m_TotFileSize += st_FileInfo.nFileSize;
				st_FileInfo.isDir = FALSE;
				_tcscpy_s(st_FileInfo.szFilePath, strSavePath);
			}

			file_list.insert(pair<CString, FileInfo>(st_FileInfo.szFilePath, st_FileInfo));//맵에 저장한다
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

//현재 exe파일이 실행되는 경로를 구하는 함수
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

//전송정보를 구함
void CSimpleFtp::GetTransferInfo(__in const TransferInfo_In& st_TransferInfo_In, __out TransferInfo_Out& st_TransferInfo_Out)
{
	int nPercent = 0, nDevide = 0;//진행정도 백분율
	DWORD nSpeed =0, nRemainTime = 0;//속도, 남은시간
	double ElapsedTime = 0;//흐른시간
	int nHour = 0, nMinute = 0, nSecond = 0;//시간, 분, 초
	
	//속도의 변동을 줄이기 위해서 초당 전송속도를 따로 계산
	if(st_TransferInfo_In.nTransferSize > 1048576)//2^20
		nDevide = 1048576;
	else if(st_TransferInfo_In.nTransferSize > 1024)//2^10
		nDevide = 1024;
	else
		nDevide = 1;
	
	if(st_TransferInfo_In.nTotFileSize > 0.0)
		nPercent = static_cast<int> (((double)st_TransferInfo_In.nTransferSize/(double)st_TransferInfo_In.nTotFileSize)*100);
	ElapsedTime = (st_TransferInfo_In.dwCurrentTime - st_TransferInfo_In.dwStartTime)/1000.0;//현재까지 흐른시간 초
	
	if(ElapsedTime >= 0.1)
		nSpeed = static_cast<int> (((st_TransferInfo_In.nTransferSize / nDevide) / ElapsedTime));//초당 속도(KB,MB)
	else
		nSpeed = 0;

	if(nSpeed > 0)
		nRemainTime = static_cast<int> ((double)(((st_TransferInfo_In.nTotFileSize - st_TransferInfo_In.nTransferSize)/nDevide))/(double)nSpeed);//남은시간
	
	if(st_TransferInfo_In.nTotFileSize == 0)
		nRemainTime = 0;
		

	nHour = static_cast<int>(floor(nRemainTime/3600.0));
	nMinute = static_cast<int>(floor(nRemainTime/60.0));


	memset(&st_TransferInfo_Out, 0, sizeof(TransferInfo_Out));
	_tcscpy_s(st_TransferInfo_Out.szFilePath, st_TransferInfo_In.szFilePath);
	st_TransferInfo_Out.nTotFileSize = st_TransferInfo_In.nTotFileSize;//총 전송할 파일크기
	st_TransferInfo_Out.nTransferSize = st_TransferInfo_In.nTransferSize;//현재 목록의 파일크기
	st_TransferInfo_Out.nSpeed = nSpeed * nDevide;//전송속도
	st_TransferInfo_Out.nPercent = nPercent;//진행정도(백분율)

	if(nHour > 0)
	{
		//남은시간을 시간, 분으로 계산한다
		nMinute = static_cast<int>((nRemainTime/3600.0 - floor(nRemainTime/3600.0))*60);
		st_TransferInfo_Out.nRemainHour = nHour;
		st_TransferInfo_Out.nRemainMin = nMinute;
	}
	else if(nHour <= 0 && nMinute > 0)
	{
		//남은시간을 분, 초로 계산한다
		nMinute = static_cast<int>(floor(nRemainTime/60.0));
		nSecond = static_cast<int>((nRemainTime/60.0 - floor(nRemainTime/60.0))*60);
		st_TransferInfo_Out.nRemainMin = nMinute;
		st_TransferInfo_Out.nRemainSec = nSecond;
	}
	else
	{
		//남은시간을 초로 계산한다
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
//			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return -1;
		}
	}

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));

	if(strRemoteDir.GetLength() == 0 || strRemoteDir.Find(_T("/")) < 0)
		return -1;//에러

	
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//사용자가 입력한 폴더경로의 마지막에 '\'가 없으면
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength()-1);//'/'를 제거(''(공백)으로 변환)

	if(FtpCreateDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)//디렉토리 생성을 시도
	{
		//디렉토리 생성 실패
//		ShowLastErrorMsg();
		return 0;
	}

	if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
		return FALSE;

	return 1;
}
//FTP서버에 디렉토리르 만듦
//상위디렉토리가 존재하지 않는경우 상위디렉토리 강제 생성
int CSimpleFtp::CreateRemoteDir(CString strRemoteDir)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return -1;
		}
	}

	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));

	if(strRemoteDir.GetLength() == 0 || strRemoteDir.Find(_T("/")) < 0)
		return -1;//에러

	
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//사용자가 입력한 폴더경로의 마지막에 '\'가 없으면
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength()-1);//'/'를 제거(''(공백)으로 변환)

	if(FindRemoteDir(strRemoteDir) == TRUE)//이미 폴더가 존재하면
		return 1;

	//strDirPath 문자열의 중간에 있는 폴더 생성
	//가령 /123/456/789 라면 /123, /123/456 폴더 생성
	for(int i = 0; i < strRemoteDir.GetLength(); i++)
	{
		if(strRemoteDir.GetAt(i) == _T('/'))
		{
			CString strCreateDir = strRemoteDir.Left(i);
			if(FindRemoteDir(strCreateDir) != 0)//검색오류 또는 이미 디렉토리가 존재하면
				continue;

			if(FtpCreateDirectoryA(hFtpConn, GetSendString(strCreateDir)) == FALSE)//디렉토리 생성을 시도
			{
				//디렉토리 생성 실패
				ShowLastErrorMsg();
				return 0;
			}

			if(WaitForSingleObject(hCloseEvent, 0) == WAIT_OBJECT_0)//예외적 종료
				return FALSE;
		}
	}

	if(FtpCreateDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)//최종 디렉토리(/123/456/789) 생성
	{
		ShowLastErrorMsg();
		return 0;
	}

	return 1;
}

//FTP서버에 디렉토리가 있는지 검색
//존재하면 1, 존재하지 않으면 0, 에러 -1
int CSimpleFtp::FindRemoteDir(CString strRemoteDir)
{
	if(IsConnected() == FALSE)
	{
		if(Login() == FALSE)
		{
			AfxMessageBox(_T("서버에 연결할 수 없습니다"));
			return FALSE;
		}
	}

	if(strRemoteDir.GetLength() == 0 )
		return  -1;
	
	strRemoteDir.Replace(_T("\\"), _T("/"));
	strRemoteDir.Replace(_T("//"), _T("/"));

	if(strRemoteDir == _T("/"))
		return 1;
	
	if(strRemoteDir.GetLength() > 1 && strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//디렉토리경로의 마지막에 '/'가 있으면
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength()-1);//'/'를 제거(''(공백)으로 변환)

	FILE_LIST file_list;
	FILE_LIST::iterator itr;
	CString strParentDir = GetParentDirFromPath(strRemoteDir);//strRemoteDir이 '/123/456/789'라면 '/123/456' 리턴
	if(GetRemoteFileList(strParentDir, file_list) == FALSE)//파일목록(디렉토리+파일)을 구함
		return 0;
	itr = file_list.find(strRemoteDir);
	if(itr != file_list.end() && itr->second.isDir == TRUE)
		return 1; //찾았음

	return 0;
}

//FTP서버에 파일이 존재하는지 검색
//존재하면 1, 존재하지 않으면 0, 에러 -1
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
	CString strParentDir = GetParentDirFromPath(strRemoteFilePath);//strRemoteFilePath이 '/123/456/789.txt'라면 '/123/456' 리턴
	GetRemoteFileList(strParentDir, file_list);//파일목록(디렉토리+파일)을 구함
	itr = file_list.find(strRemoteFilePath);
	if(itr != file_list.end() && itr->second.isDir == FALSE)
		return 1; //찾았음

	return 0;//못찾았음
}

//파일 또는 디렉토리 이름변경
BOOL CSimpleFtp::RenameRemoteFile(CString strRemoteOldPath, CString strRemoteNewPath)
{

	if(FtpRenameFileA(hFtpConn, GetSendString(strRemoteOldPath), GetSendString(strRemoteNewPath)) == FALSE)
	{
		ShowLastErrorMsg();
		return FALSE;
	}
	
	return TRUE;
}

//FTP서버의 하위디렉토리 포함 삭제
BOOL CSimpleFtp::DeleteRemoteDir(CString strRemoteDir)
{

	if(strRemoteDir.GetLength() == 0)
		return FALSE;
	strRemoteDir.Replace(_T("\\"), _T("/"));

	if(strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//사용자가 입력한 폴더경로의 마지막에 '\'또는 '/'가 있으면
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1); //제거
	if(strRemoteDir.GetAt(0) != _T('/'))
		strRemoteDir = _T("/") + strRemoteDir;

	FILE_LIST del_list;
	FILE_LIST::reverse_iterator itr;//역순 반복자

	//삭제목록을 구함
	if(GetRemoteFileListWithSubDir(strRemoteDir, del_list) == FALSE)
		return FALSE;	//삭제목록 구하기 실패

	
	//del_list에 저장된 역순으로 삭제(하위디렉토리부터 삭제해야 하므로)
	for(itr = del_list.rbegin(); itr != del_list.rend(); itr++)
	{
		if(itr->second.isDir)//디렉토리이면
		{
			//디렉토리 삭제
			if(FtpRemoveDirectoryA(hFtpConn, GetSendString(itr->second.szFilePath)) == FALSE)
			{
				ShowLastErrorMsg();
				return FALSE;
			}

		}
		else//파일이면
		{
			if(DeleteRemoteFile(itr->second.szFilePath) == FALSE)//파일 삭제
				return FALSE;
		}
	}

	if(FtpSetCurrentDirectoryA(hFtpConn, GetSendString(GetParentDirFromPath(strRemoteDir))) == FALSE)//알FTP 서버는 디렉토리를 이동하고 리스트를 구해야 한다
	{
		ShowLastErrorMsg();
		return FALSE;//디렉토리 이동 실패
	}

	//최종 상위 디렉토리 삭제
	if(FtpRemoveDirectoryA(hFtpConn, GetSendString(strRemoteDir)) == FALSE)
	{
		ShowLastErrorMsg();
		return FALSE;
	}

	return TRUE;
}

//FTP서버의 파일 삭제
BOOL CSimpleFtp::DeleteRemoteFile(CString strRemoteFilePath)
{
	if(strRemoteFilePath.GetLength() == 0)
		return FALSE;
	strRemoteFilePath.Replace(_T("\\"), _T("/"));

	if(strRemoteFilePath.GetAt(strRemoteFilePath.GetLength()-1) == _T('/'))//사용자가 입력한 폴더경로의 마지막에 '\'또는 '/'가 있으면
		strRemoteFilePath = strRemoteFilePath.Left(strRemoteFilePath.GetLength() - 1); //제거

	if(strRemoteFilePath.GetAt(0) != _T('/'))
		strRemoteFilePath = _T("/") + strRemoteFilePath;

	
	if(FtpSetCurrentDirectoryA(hFtpConn, GetSendString(GetParentDirFromPath(strRemoteFilePath))) == FALSE)//알FTP 서버는 디렉토리를 이동하고 리스트를 구해야 한다
	{
		ShowLastErrorMsg();
		return FALSE;//디렉토리 이동 실패
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
	
	if(strRemoteDir.GetAt(strRemoteDir.GetLength()-1) == _T('/'))//사용자가 입력한 폴더경로의 마지막에 '/'가 있으면
		strRemoteDir = strRemoteDir.Left(strRemoteDir.GetLength() - 1); //제거

	
	return TRUE;
}

//FTP 에러발생시 에러메시지를 보여줌
void CSimpleFtp::ShowLastErrorMsg()
{
	TCHAR szErrorMsg[1024]={0,};
	DWORD nBuffSize = 1023, nErrorCode = 0;
	InternetGetLastResponseInfo(&nErrorCode, szErrorMsg, &nBuffSize);
	if(_tcslen(szErrorMsg) > 0)
		AfxMessageBox(szErrorMsg);
}

//FTP서버에 따라 utf8을 지원하는 서버가 있고 지원하지 않는 서버가 있음
//Login할때 인자값으로 bUtf8을 서버에 맞게 설정하면 자동으로 동작함
CStringA CSimpleFtp::GetSendString(CString strSend)
{
	CStringA strSendA;
	CStringW strSendW;

#ifdef UNICODE
	if(bUtf8)
		strSendA = CW2A(strSend, CP_UTF8);//문자열을 UTF8로 인코딩
	else
		strSendA = CW2A(strSend);
#else
	strSendW = CA2W(strSend);
	if(bUtf8)
		strSendA = CW2A(strSendW,  CP_UTF8);//문자열을 UTF8로 인코딩
	else
		strSendA = strSend;
#endif
	
	return strSendA;
}

//c:\123\456.txt라면 456.txt를 리턴
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

	if(strFilePath.GetAt(strFilePath.GetLength()-1) == szSeperator)//사용자가 입력한 폴더경로의 마지막에 '\'또는 '/'가 있으면
		strFilePath = strFilePath.Left(strFilePath.GetLength() - 1); //제거

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

//c:\123\456\789.txt라면 c:\123\456을 리턴
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

	if(strFilePath.GetAt(strFilePath.GetLength()-1) == szSeperator)//사용자가 입력한 폴더경로의 마지막에 '\'또는 '/'가 있으면
		strFilePath = strFilePath.Left(strFilePath.GetLength() - 1); //제거


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

//이어올리기 용도로 서버의 파일을 연다
BOOL CSimpleFtp::ReadyUploadResume(CString strRemoteFilePath, UINT64 nStartFilePointer)
{
	CString strCommand;
	
	FtpSetCurrentDirectoryA(hFtpConn, GetSendString(GetParentDirFromPath(strRemoteFilePath)));

	strCommand.Format(_T("REST %I64d"), nStartFilePointer);
	if(FtpCommandA(hFtpConn, FALSE, FTP_TRANSFER_TYPE_BINARY, GetSendString(strCommand), 0, 0) == FALSE)
		return FALSE;//이어올리기 지원 안함
	if(IsPositiveResponse() == FALSE)
		return FALSE;
	
	return TRUE;
}

//이어올리기 용도로 서버의 파일을 연다
//APPE를 사용하면 좋으나 APPE를 이용하려면 PORT 명령어를 먼저 사용해야 하는 듯 하다
//문제는 PORT 명령어를 사용하려면 현재 FTP 접속 소켓의 열린 port를 알아야 하는데 그게 안된다
//MSDN에는 서버 접속 성공하면 콜백함수로 SOCKADDR 포인터가 넘어온다고 되어있는데 안넘어오는 것 같다..ㅠ.ㅠ
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


//명령에 따른 서버의 반응이 긍정인지, 부정적인지?
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


//인터넷 상태를 알려주는 콜백함수
//현재 dwContext로 CSimpleFtp 클래스포인터를 가져오는 형태로 구성했다
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
	case INTERNET_STATUS_CONNECTION_CLOSED://서버와 연결이 끊김
		pSimpleFtp = (CSimpleFtp*)dwContext;
		pSimpleFtp->LogOut();//로그아웃 처리
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

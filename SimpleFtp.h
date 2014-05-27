/*****************************************************************************************************************
만든이 : 원상영

히스토리
 2009.08.27 : 최초버전
 2009.09.03 : 이어받기/올리기 기능 추가
 2009.09.08 : Login, GetRemoteFileSize, 콜백함수 등 일부 버그 수정
 2010.05.23 : ① 함수명을 일관성 있도록 변경(SetCurrentDirctory -> SetRemoteDirectory)
              ② 오타 함수명 수정(UploadWidthSubDir -> UploadWithSubDir)
              ③ 수천개의 파일 Upload 중에 사용자가 강제로 중지시킨경우 멈추는 문제 수정

 받는곳 : http://blog.daum.net/odega
*****************************************************************************************************************/



#pragma once

#include <WinInet.h>
#include <map>

using namespace std;


#define WM_SHOW_DOWNLOAD_STATE WM_USER+6683
#define WM_SHOW_UPLOAD_STATE WM_USER+6684

#define MAX_BUFFER_SIZE 65536 

#define REST 0 //이어받기/올리기
#define APPE 1 //파일끝에 이어올리기

class CSimpleFtp
{
public:
	CSimpleFtp(void);
	~CSimpleFtp(void);


public:
	HINTERNET hSession;//인터넷 세션
	HINTERNET hFtpConn;//FTP 세션
	//업로드 또는 다운로드 파일 정보
	struct FileInfo
	{
		TCHAR szFilePath[1024];//경로 + 파일명
		ULONGLONG nFileSize;//파일크기
		BOOL isDir;//디렉토리 여부
		SYSTEMTIME stCreattionTime, stLastAccessTime, stLastWriteTime;//생성날짜, 최종접근일, 최종수정일
	};
	typedef map <CString, FileInfo> FILE_LIST;//업로드 또는 다운로드 목록(파일 및 디렉토리)을 구함
	
	//전송중인 데이터 정보
	struct TransferInfo_In
	{
		TCHAR szFilePath[1024];//전송중인 파일명(경로+파일명)
		DWORD dwStartTime, dwCurrentTime;//전송시작시간, 전송현재시간(전송시작시간으로부터 경과한 시간을 구하는데 이용)
		UINT64 nTotFileSize;//전송할 데이터의 총 크기
		UINT64 nTransferSize;//현재 전송된 데이터 크기
	};
	
	struct TransferInfo_Out
	{
		TCHAR szFilePath[1024];//전송중인 파일명(경로+파일명)
		UINT64 nTotFileSize;//전송할 데이터 총 크기
		UINT64 nTransferSize;//현재 전송된 데이터 크기
		int nRemainHour;//남은 시간
		int nRemainMin;//남은 분
		int nRemainSec;//남은 초
		int nSpeed; //전송속도(초당 Byte). KB로 표시하려면 1024로 나누면 된다
		int nPercent;//전송율(백분율로 표시한다)
	};

private:
	HWND hCallWnd;//호출한 부모윈도의 핸들. 현재 진행중인 상태를 메시지로 부모윈도에 보낼때 사용한다
	HANDLE hCloseEvent;//예외적 종료. 전송중에 사용자가 취소하거나 서버로부터 강제종료된 경우 예외처리를 하는데 이용한다
	BOOL bUtf8;//UTF8로 데이터(파일명, 디렉토리명) 전송
	CString strIP, strID, strPasswd;//접속아이피, 아이디, 암호, Passive/Active모드
	UINT nPort;//FTP 접속 포트(기본 21)
	BOOL bPassive;

public:
	//인터넷 상태(연결중, 연결완료, 연결종료중, 연결종료 등)의 이벤트를 받는 콜백함수.
	friend void __stdcall CallMaster( HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);
	
	//로그인
	BOOL Login(CString strIP, CString strID=_T("anonymous"), CString strPasswd=_T(""), 
		UINT nPort = INTERNET_DEFAULT_FTP_PORT, HWND hCallWnd = NULL, BOOL bUtf8 = TRUE, BOOL bPassive = FALSE);
	//로그아웃
	void LogOut();
	//FTP서버에 연결되어있으면 TRUE, 아니면 FALSE
	BOOL IsConnected();
	//파일만 다운로드
	BOOL DownloadFile(CString strRemoteFilePath, CString strLocalFilePath, BOOL bResume = FALSE);
	//하위 디렉토리까지 포함하여 다운로드
	BOOL DownloadWithSubDir(CString strRemoteDir, CString strLocalDir, BOOL bResume = FALSE);
	//파일만 업로드
	BOOL UploadFile(CString strLocalFilePath, CString strRemoteFilePath, BOOL bResume = FALSE);
	//하위디렉토리포함 업로드
	BOOL UploadWithSubDir(CString strLocalDir, CString strRemoteDir, BOOL bResume = FALSE);
	//FTP서버에 디렉토리를 생성(서버에 없는 디렉토리는 생성한다. 가령 strRemoteDir이 /123/456/789인 경우 /123, /123/456 디렉토리가 없다면 생성한다)
	int CreateRemoteDir(CString strRemoteDir);
	int CreateRemoteDir2(CString strRemoteDir);
	//FTP서버 파일 또는 디렉토리 이름변경
	BOOL RenameRemoteFile(CString strRemoteOldPath, CString strRemoteNewPath);
	//FTP서버의 하위디렉토리 포함 삭제
	BOOL DeleteRemoteDir(CString strRemoteDir);
	//FTP서버의 파일 삭제
	BOOL DeleteRemoteFile(CString strRemoteFilePath);
	//FTP서버 현재 디렉토리 변경
	BOOL SetRemoteDirectory(CString strRemoteDir);
	//FTP서버의 현재 디렉토리 경로를 구함
	BOOL GetRemoteDirectory(__out CString& strRemoteDir);
	//FTP서버의 파일 크기를 구함(성공 TRUE, 실패 FALSE)
	BOOL GetRemoteFileSize(__in CString strRemoteFilePath, __out UINT64& nRemoteFileSize);
	//FTP서버에 특정 디렉토리가 있는지 확인. FTP서버에 디렉토리가 존재하면 1, 오류인경우 -1, 없으면 0
	int FindRemoteDir(CString strRemoteDir);
	//FTP서버에 특정 파일이 있는지 확인. FTP서버에 파일이 존재하면 존재하면 1, 오류인경우 -1, 없으면 0
	int FindRemoteFile(CString strRemoteFilePath);
	//하위 디렉토리 포함한 파일목록(디렉토리+파일)을 구함
	BOOL GetRemoteFileListWithSubDir(__in CString strRemoteDir, __out FILE_LIST& file_list);
	//FTP의 디렉토리 파일목록(디렉토리+파일)을 구함. 하위디렉토리 미포함
	BOOL GetRemoteFileList(__in CString strRemoteDir, __out FILE_LIST& file_list);
	
protected:
	void ShowLastErrorMsg();//FTP 에러메시지 표시
	BOOL ReadyDownloadResume(UINT64 nStartFilePointer);//이어받기 준비
	BOOL ReadyUploadResume(CString strRemoteFilePath, UINT64 nStartFilePointer);//이어올리기 준비
	BOOL IsPositiveResponse();//서버로부터의 응답이 긍정적인지 부정적인지?


//FTP 핸들을 사용하지 않는 유틸리티성 함수들
public:
	//업로드할 목록(하위디렉토리 + 파일)을 구함
	UINT64 GetLocalFileListWithSubDir(__in CString strLocalDir, __out FILE_LIST& file_list);
	//하위디렉토리 포함하여 디렉토리를 생성한다
	//가령 strDirPath가 c:\123\456\789라면 c:\, c:\123, c:\123\456, c:\123\456\789 폴더를 생성한다
	int CreateLocalDir(CString strDirPath);
	UINT64 GetTotFileSize(__in FILE_LIST& file_list);//file_list에 저장된 파일의 총 크기를 리턴
	CString GetWorkingDir();//현재 exe파일이 실행되는 경로를 구하는 함수
	
protected:
	BOOL Login();//연결이 끊겼을 경우 자동로그인(사용자가 처음 로그인할때 입력한 정보를 이용한다)
	//file_list에 파일정보(FileInfo)를 추가한다
	CString AddRemoteFile(__in const CString strCurrentRemoteDir, __in const WIN32_FIND_DATAA& find_data, __out FILE_LIST& file_list);
	//TransferInfo_In 정보를 이용하여 전송중인 파일이름, 전송속도, 남은시간, 전송데이터 크기를 구함
	void GetTransferInfo(__in const TransferInfo_In& st_TransferInfo_In, __out TransferInfo_Out& st_TransferInfo_Out);
	CStringA GetSendString(CString strSend);//파일명, 디렉토리명을 FTP로 보낼때 적절하게 문자열 인코딩
	CString GetFileNameFromPath(CString strFilePath);//c:\123\456.txt라면 456.txt를 리턴
	CString GetParentDirFromPath(CString strFilePath);//c:\123\456.txt라면 c:\123을 리턴

};

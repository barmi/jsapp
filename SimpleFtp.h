/*****************************************************************************************************************
������ : ����

�����丮
 2009.08.27 : ���ʹ���
 2009.09.03 : �̾�ޱ�/�ø��� ��� �߰�
 2009.09.08 : Login, GetRemoteFileSize, �ݹ��Լ� �� �Ϻ� ���� ����
 2010.05.23 : �� �Լ����� �ϰ��� �ֵ��� ����(SetCurrentDirctory -> SetRemoteDirectory)
              �� ��Ÿ �Լ��� ����(UploadWidthSubDir -> UploadWithSubDir)
              �� ��õ���� ���� Upload �߿� ����ڰ� ������ ������Ų��� ���ߴ� ���� ����

 �޴°� : http://blog.daum.net/odega
*****************************************************************************************************************/



#pragma once

#include <WinInet.h>
#include <map>

using namespace std;


#define WM_SHOW_DOWNLOAD_STATE WM_USER+6683
#define WM_SHOW_UPLOAD_STATE WM_USER+6684

#define MAX_BUFFER_SIZE 65536 

#define REST 0 //�̾�ޱ�/�ø���
#define APPE 1 //���ϳ��� �̾�ø���

class CSimpleFtp
{
public:
	CSimpleFtp(void);
	~CSimpleFtp(void);


public:
	HINTERNET hSession;//���ͳ� ����
	HINTERNET hFtpConn;//FTP ����
	//���ε� �Ǵ� �ٿ�ε� ���� ����
	struct FileInfo
	{
		TCHAR szFilePath[1024];//��� + ���ϸ�
		ULONGLONG nFileSize;//����ũ��
		BOOL isDir;//���丮 ����
		SYSTEMTIME stCreattionTime, stLastAccessTime, stLastWriteTime;//������¥, ����������, ����������
	};
	typedef map <CString, FileInfo> FILE_LIST;//���ε� �Ǵ� �ٿ�ε� ���(���� �� ���丮)�� ����
	
	//�������� ������ ����
	struct TransferInfo_In
	{
		TCHAR szFilePath[1024];//�������� ���ϸ�(���+���ϸ�)
		DWORD dwStartTime, dwCurrentTime;//���۽��۽ð�, ��������ð�(���۽��۽ð����κ��� ����� �ð��� ���ϴµ� �̿�)
		UINT64 nTotFileSize;//������ �������� �� ũ��
		UINT64 nTransferSize;//���� ���۵� ������ ũ��
	};
	
	struct TransferInfo_Out
	{
		TCHAR szFilePath[1024];//�������� ���ϸ�(���+���ϸ�)
		UINT64 nTotFileSize;//������ ������ �� ũ��
		UINT64 nTransferSize;//���� ���۵� ������ ũ��
		int nRemainHour;//���� �ð�
		int nRemainMin;//���� ��
		int nRemainSec;//���� ��
		int nSpeed; //���ۼӵ�(�ʴ� Byte). KB�� ǥ���Ϸ��� 1024�� ������ �ȴ�
		int nPercent;//������(������� ǥ���Ѵ�)
	};

private:
	HWND hCallWnd;//ȣ���� �θ������� �ڵ�. ���� �������� ���¸� �޽����� �θ������� ������ ����Ѵ�
	HANDLE hCloseEvent;//������ ����. �����߿� ����ڰ� ����ϰų� �����κ��� ��������� ��� ����ó���� �ϴµ� �̿��Ѵ�
	BOOL bUtf8;//UTF8�� ������(���ϸ�, ���丮��) ����
	CString strIP, strID, strPasswd;//���Ӿ�����, ���̵�, ��ȣ, Passive/Active���
	UINT nPort;//FTP ���� ��Ʈ(�⺻ 21)
	BOOL bPassive;

public:
	//���ͳ� ����(������, ����Ϸ�, ����������, �������� ��)�� �̺�Ʈ�� �޴� �ݹ��Լ�.
	friend void __stdcall CallMaster( HINTERNET, DWORD_PTR, DWORD, LPVOID, DWORD);
	
	//�α���
	BOOL Login(CString strIP, CString strID=_T("anonymous"), CString strPasswd=_T(""), 
		UINT nPort = INTERNET_DEFAULT_FTP_PORT, HWND hCallWnd = NULL, BOOL bUtf8 = TRUE, BOOL bPassive = FALSE);
	//�α׾ƿ�
	void LogOut();
	//FTP������ ����Ǿ������� TRUE, �ƴϸ� FALSE
	BOOL IsConnected();
	//���ϸ� �ٿ�ε�
	BOOL DownloadFile(CString strRemoteFilePath, CString strLocalFilePath, BOOL bResume = FALSE);
	//���� ���丮���� �����Ͽ� �ٿ�ε�
	BOOL DownloadWithSubDir(CString strRemoteDir, CString strLocalDir, BOOL bResume = FALSE);
	//���ϸ� ���ε�
	BOOL UploadFile(CString strLocalFilePath, CString strRemoteFilePath, BOOL bResume = FALSE);
	//�������丮���� ���ε�
	BOOL UploadWithSubDir(CString strLocalDir, CString strRemoteDir, BOOL bResume = FALSE);
	//FTP������ ���丮�� ����(������ ���� ���丮�� �����Ѵ�. ���� strRemoteDir�� /123/456/789�� ��� /123, /123/456 ���丮�� ���ٸ� �����Ѵ�)
	int CreateRemoteDir(CString strRemoteDir);
	int CreateRemoteDir2(CString strRemoteDir);
	//FTP���� ���� �Ǵ� ���丮 �̸�����
	BOOL RenameRemoteFile(CString strRemoteOldPath, CString strRemoteNewPath);
	//FTP������ �������丮 ���� ����
	BOOL DeleteRemoteDir(CString strRemoteDir);
	//FTP������ ���� ����
	BOOL DeleteRemoteFile(CString strRemoteFilePath);
	//FTP���� ���� ���丮 ����
	BOOL SetRemoteDirectory(CString strRemoteDir);
	//FTP������ ���� ���丮 ��θ� ����
	BOOL GetRemoteDirectory(__out CString& strRemoteDir);
	//FTP������ ���� ũ�⸦ ����(���� TRUE, ���� FALSE)
	BOOL GetRemoteFileSize(__in CString strRemoteFilePath, __out UINT64& nRemoteFileSize);
	//FTP������ Ư�� ���丮�� �ִ��� Ȯ��. FTP������ ���丮�� �����ϸ� 1, �����ΰ�� -1, ������ 0
	int FindRemoteDir(CString strRemoteDir);
	//FTP������ Ư�� ������ �ִ��� Ȯ��. FTP������ ������ �����ϸ� �����ϸ� 1, �����ΰ�� -1, ������ 0
	int FindRemoteFile(CString strRemoteFilePath);
	//���� ���丮 ������ ���ϸ��(���丮+����)�� ����
	BOOL GetRemoteFileListWithSubDir(__in CString strRemoteDir, __out FILE_LIST& file_list);
	//FTP�� ���丮 ���ϸ��(���丮+����)�� ����. �������丮 ������
	BOOL GetRemoteFileList(__in CString strRemoteDir, __out FILE_LIST& file_list);
	
protected:
	void ShowLastErrorMsg();//FTP �����޽��� ǥ��
	BOOL ReadyDownloadResume(UINT64 nStartFilePointer);//�̾�ޱ� �غ�
	BOOL ReadyUploadResume(CString strRemoteFilePath, UINT64 nStartFilePointer);//�̾�ø��� �غ�
	BOOL IsPositiveResponse();//�����κ����� ������ ���������� ����������?


//FTP �ڵ��� ������� �ʴ� ��ƿ��Ƽ�� �Լ���
public:
	//���ε��� ���(�������丮 + ����)�� ����
	UINT64 GetLocalFileListWithSubDir(__in CString strLocalDir, __out FILE_LIST& file_list);
	//�������丮 �����Ͽ� ���丮�� �����Ѵ�
	//���� strDirPath�� c:\123\456\789��� c:\, c:\123, c:\123\456, c:\123\456\789 ������ �����Ѵ�
	int CreateLocalDir(CString strDirPath);
	UINT64 GetTotFileSize(__in FILE_LIST& file_list);//file_list�� ����� ������ �� ũ�⸦ ����
	CString GetWorkingDir();//���� exe������ ����Ǵ� ��θ� ���ϴ� �Լ�
	
protected:
	BOOL Login();//������ ������ ��� �ڵ��α���(����ڰ� ó�� �α����Ҷ� �Է��� ������ �̿��Ѵ�)
	//file_list�� ��������(FileInfo)�� �߰��Ѵ�
	CString AddRemoteFile(__in const CString strCurrentRemoteDir, __in const WIN32_FIND_DATAA& find_data, __out FILE_LIST& file_list);
	//TransferInfo_In ������ �̿��Ͽ� �������� �����̸�, ���ۼӵ�, �����ð�, ���۵����� ũ�⸦ ����
	void GetTransferInfo(__in const TransferInfo_In& st_TransferInfo_In, __out TransferInfo_Out& st_TransferInfo_Out);
	CStringA GetSendString(CString strSend);//���ϸ�, ���丮���� FTP�� ������ �����ϰ� ���ڿ� ���ڵ�
	CString GetFileNameFromPath(CString strFilePath);//c:\123\456.txt��� 456.txt�� ����
	CString GetParentDirFromPath(CString strFilePath);//c:\123\456.txt��� c:\123�� ����

};

#pragma once
#include <Windows.h>
#include <tchar.h>

#define Native_MSG_BUFF_SIZE 4096

extern const TCHAR* g_pszPipename;
//extern const UINT g_nNativeMsgBufSize;

//todo:这里以后要不要换成其它开源通信模块？
//todo:要不要每种通信模式都尝试下
class CPipeMgr
{
public:
	class CPipeClient
	{
	public:
		BOOL CreatePipeClient();
	};

public:
	class CPipeServer
	{

	public:
		typedef struct
		{
			OVERLAPPED oOverlap;
			HANDLE hPipeInst;
			TCHAR chRequest[Native_MSG_BUFF_SIZE];
			DWORD cbRead;
			TCHAR chReply[Native_MSG_BUFF_SIZE];
			DWORD cbToWrite;
			CPipeMgr::CPipeServer* pThis;
		} PIPEINST, * LPPIPEINST;

		//直接参考的：https://learn.microsoft.com/en-us/windows/win32/ipc/named-pipe-server-using-completion-routines
		//APC调用和完成端口搭配接受管道信息
		BOOL CreatePipeServerWithCompRout();
	private:
		BOOL CreateAndConnectInstance(LPOVERLAPPED lpoOverlap);
		BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo);
		VOID DisconnectAndClose(LPPIPEINST lpPipeInst);
		static VOID WINAPI CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
		static VOID WINAPI CompletedWriteRoutine(DWORD dwErr, DWORD cbWritten, LPOVERLAPPED lpOverLap);
		VOID GetAnswerToRequest(LPPIPEINST pipe);
	private:
		HANDLE m_hPipe;
	};
};


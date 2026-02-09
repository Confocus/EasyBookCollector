#include "PipeMgr.h"
#include <strsafe.h>

const TCHAR* g_pszPipename = _T("\\\\.\\pipe\\mynamedpipe{45691F69-29D1-444F-8EC7-D1DCB73097F3}");
const UINT g_nTimeOut = 5000;

VOID CPipeMgr::CPipeServer::GetAnswerToRequest(LPPIPEINST pipe)
{
	//_tprintf(TEXT("[%d] %s\n"), pipe->hPipeInst, pipe->chRequest);
	StringCchCopy(pipe->chReply, Native_MSG_BUFF_SIZE, TEXT("Default answer from server"));
	pipe->cbToWrite = (lstrlen(pipe->chReply) + 1) * sizeof(TCHAR);
}

// DisconnectAndClose(LPPIPEINST) 
// This routine is called when an error occurs or the client closes 
// its handle to the pipe. 

VOID CPipeMgr::CPipeServer::DisconnectAndClose(LPPIPEINST lpPipeInst)
{
	// Disconnect the pipe instance. 

	if (!DisconnectNamedPipe(lpPipeInst->hPipeInst))
	{
		//printf("DisconnectNamedPipe failed with %d.\n", GetLastError());
	}

	// Close the handle to the pipe instance. 

	CloseHandle(lpPipeInst->hPipeInst);

	// Release the storage for the pipe instance. 

	if (lpPipeInst != NULL)
		GlobalFree(lpPipeInst);
}

VOID WINAPI CPipeMgr::CPipeServer::CompletedReadRoutine(DWORD dwErr, DWORD cbBytesRead,
	LPOVERLAPPED lpOverLap)
{
	LPPIPEINST lpPipeInst;
	BOOL fWrite = FALSE;

	// lpOverlap points to storage for this instance. 

	lpPipeInst = (LPPIPEINST)lpOverLap;

	// The read operation has finished, so write a response (if no 
	// error occurred). 
	
	if ((dwErr == 0) && (cbBytesRead != 0))
	{
		lpPipeInst->pThis->GetAnswerToRequest(lpPipeInst);
		fWrite = WriteFileEx(
			lpPipeInst->hPipeInst,
			lpPipeInst->chReply,
			lpPipeInst->cbToWrite,
			(LPOVERLAPPED)lpPipeInst,
			(LPOVERLAPPED_COMPLETION_ROUTINE)CompletedWriteRoutine);
		//刚才直行道这里，然后异步了，进入alertable状态
		//alterable状态回来执行CompletedWriteRoutine
		//再次进入CompletedWriteRoutine会ReadFileEx失败然后直接DisconnectAndClose吧
	}

	// Disconnect if an error occurred. 

	if (!fWrite)
		lpPipeInst->pThis->DisconnectAndClose(lpPipeInst);
}

VOID WINAPI CPipeMgr::CPipeServer::CompletedWriteRoutine(DWORD dwErr, DWORD cbWritten,
	LPOVERLAPPED lpOverLap)
{
	LPPIPEINST lpPipeInst;
	BOOL fRead = FALSE;

	// lpOverlap points to storage for this instance. 

	lpPipeInst = (LPPIPEINST)lpOverLap;

	// The write operation has finished, so read the next request (if 
	// there is no error). 
	if ((dwErr == 0) && (cbWritten == lpPipeInst->cbToWrite))
		fRead = ReadFileEx(
			lpPipeInst->hPipeInst,
			lpPipeInst->chRequest,
			Native_MSG_BUFF_SIZE * sizeof(TCHAR),
			(LPOVERLAPPED)lpPipeInst,
			(LPOVERLAPPED_COMPLETION_ROUTINE)CompletedReadRoutine);
			//如果读完了会执行CompletedReadRoutine，
			//CompletedReadRoutine里调用了WriteFileEx等待WriteFile完成进入alertable

	// Disconnect if an error occurred. 

	if (!fRead)
		lpPipeInst->pThis->DisconnectAndClose(lpPipeInst);
}

BOOL CPipeMgr::CPipeServer::CreatePipeServerWithCompRout()
{
	HANDLE hConnectEvent;
	OVERLAPPED oConnect;
	LPPIPEINST lpPipeInst;
	DWORD dwWait, cbRet;
	BOOL fSuccess, fPendingIO;
	// Create one event object for the connect operation. 
	hConnectEvent = CreateEvent(
		NULL,    // default security attribute
		TRUE,    // manual reset event 
		TRUE,    // initial state = signaled 
		NULL);   // unnamed event object 

	if (hConnectEvent == NULL)
	{
		//printf("CreateEvent failed with %d.\n", GetLastError());
		return 0;
	}

	oConnect.hEvent = hConnectEvent;

	// Call a subroutine to create one instance, and wait for 
	// the client to connect. 

	fPendingIO = CreateAndConnectInstance(&oConnect);

	while (1)
	{
		// Wait for a client to connect, or for a read or write 
		// operation to be completed, which causes a completion 
		// routine to be queued for execution. 
		//这里进入alertable状态
		dwWait = WaitForSingleObjectEx(
			hConnectEvent,  // event object to wait for 
			INFINITE,       // waits indefinitely 
			TRUE);          // alertable wait enabled 

		switch (dwWait)
		{
			// The wait conditions are satisfied by a completed connect 
			// operation. 
		case 0:
			// If an operation is pending, get the result of the 
			// connect operation. 
			if (fPendingIO)
			{
				fSuccess = GetOverlappedResult(
					m_hPipe,     // pipe handle 
					&oConnect, // OVERLAPPED structure 
					&cbRet,    // bytes transferred 
					FALSE);    // does not wait 
				if (!fSuccess)
				{
					//printf("ConnectNamedPipe (%d)\n", GetLastError());
					return 0;
				}
			}

			// Allocate storage for this instance. 
			lpPipeInst = (LPPIPEINST)GlobalAlloc(
				GPTR, sizeof(PIPEINST));
			if (lpPipeInst == NULL)
			{
				//printf("GlobalAlloc failed (%d)\n", GetLastError());
				return 0;
			}

			lpPipeInst->hPipeInst = m_hPipe;
			lpPipeInst->pThis = this;
			// Start the read operation for this client. 
			// Note that this same routine is later used as a 
			// completion routine after a write operation. 

			lpPipeInst->cbToWrite = 0;
			CompletedWriteRoutine(0, 0, (LPOVERLAPPED)lpPipeInst);
			//相当于就是处理着这次读写请求的同时创建了一个新端口
			// Create new pipe instance for the next client. 
			fPendingIO = CreateAndConnectInstance(
				&oConnect);
			break;

			// The wait is satisfied by a completed read or write 
			// operation. This allows the system to execute the 
			// completion routine. 
			//如果是一次读或写的完成
		case WAIT_IO_COMPLETION:
			break;

			// An error occurred in the wait function. 

		default:
		{
			//printf("WaitForSingleObjectEx (%d)\n", GetLastError());
			return 0;
		}
		}
	}
	return 0;
}

BOOL CPipeMgr::CPipeServer::CreateAndConnectInstance(LPOVERLAPPED lpoOverlap)
{
	LPCTSTR lpszPipename = g_pszPipename;

	m_hPipe = CreateNamedPipe(
		lpszPipename,             // pipe name 
		PIPE_ACCESS_DUPLEX |      // read/write access 
		FILE_FLAG_OVERLAPPED,     // overlapped mode 
		PIPE_TYPE_MESSAGE |       // message-type pipe 
		PIPE_READMODE_MESSAGE |   // message read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // unlimited instances 
		Native_MSG_BUFF_SIZE * sizeof(TCHAR),    // output buffer size 
		Native_MSG_BUFF_SIZE * sizeof(TCHAR),    // input buffer size 
		g_nTimeOut,             // client time-out 
		NULL);                    // default security attributes
	if (m_hPipe == INVALID_HANDLE_VALUE)
	{
		//printf("CreateNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}

	// Call a subroutine to connect to the new client. 

	return ConnectToNewClient(m_hPipe, lpoOverlap);
}

BOOL CPipeMgr::CPipeServer::ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;

	// Start an overlapped connection for this pipe instance. 
	fConnected = ConnectNamedPipe(hPipe, lpo);

	// Overlapped ConnectNamedPipe should return zero. 
	if (fConnected)
	{
		//printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}

	switch (GetLastError())
	{
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING:
		fPendingIO = TRUE;
		break;

		// Client is already connected, so signal an event. 

	case ERROR_PIPE_CONNECTED:
		if (SetEvent(lpo->hEvent))
			break;

		// If an error occurs during the connect operation... 
	default:
	{
		//printf("ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}
	}
	return fPendingIO;
}

BOOL CPipeMgr::CPipeClient::CreatePipeClient()
{
	HANDLE hPipe;
	LPCTSTR lpvMessage = TEXT("Default message from client.");
	TCHAR  chBuf[Native_MSG_BUFF_SIZE];
	BOOL   fSuccess = FALSE;
	DWORD  cbRead, cbToWrite, cbWritten, dwMode;
	LPCTSTR lpszPipename = g_pszPipename;

	/*if (argc > 1)
		lpvMessage = argv[1];*/

	// Try to open a named pipe; wait for it, if necessary. 
	while (1)
	{
		hPipe = CreateFile(
			lpszPipename,   // pipe name 
			GENERIC_READ |  // read and write access 
			GENERIC_WRITE,
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 

		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
			return -1;
		}

		// All pipe instances are busy, so wait for 20 seconds. 
		if (!WaitNamedPipe(lpszPipename, 20000))
		{
			printf("Could not open pipe: 20 second wait timed out.");
			return -1;
		}
	}

	// The pipe connected; change to message-read mode. 
	//说明 你的后续逻辑假设的是：“一次 ReadFile = 一条完整消息”
	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipe,    // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	if (!fSuccess)
	{
		_tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
		return -1;
	}

	// Send a message to the pipe server. 
	cbToWrite = (lstrlen(lpvMessage) + 1) * sizeof(TCHAR);
	_tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage);

	fSuccess = WriteFile(
		hPipe,                  // pipe handle 
		lpvMessage,             // message 
		cbToWrite,              // message length 
		&cbWritten,             // bytes written 
		NULL);                  // not overlapped 

	if (!fSuccess)
	{
		_tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
		return -1;
	}

	printf("\nMessage sent to server, receiving reply as follows:\n");
	do
	{
		// Read from the pipe. 

		fSuccess = ReadFile(
			hPipe,    // pipe handle 
			chBuf,    // buffer to receive reply 
			Native_MSG_BUFF_SIZE * sizeof(TCHAR),  // size of buffer 
			&cbRead,  // number of bytes read 
			NULL);    // not overlapped 

		if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
			break;

		_tprintf(TEXT("\"%s\"\n"), chBuf);
	} while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

	if (!fSuccess)
	{
		_tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
		return -1;
	}

	printf("\n<End of message, press ENTER to terminate connection and exit>");
	//_getch();

	CloseHandle(hPipe);

	return 0;
}

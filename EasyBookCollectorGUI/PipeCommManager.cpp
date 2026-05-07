#include "PipeCommManager.h"

#define PIPE_NAME_BOOKMARK_TRANS	L"\\\\.\\pipe\\BookmarkTransPipe"
#define EVENT_NAME_SENT_RECV_CMD	L"{31E3A6F1-105A-45D9-8E73-79CE24064F5C}\SendRecvCmd"
#define EVENT_NAME_RESPONSE	L"{A7486818-B995-4F67-BA45-834BE0B980EC}\Response"
#define EVENT_NAME_CONNECT_PIPE	L"{A1418B8A-7998-4262-9D44-47E607653E93}\ConnectPipe"
#define EVENT_NAME_DISCONNECT_PIPE	L"{4E17318B-F76A-448B-8401-42085E3AC90D}\DisconnectPipe"
#define MAX_CMD_LEN	256
#define PIPE_READ_LEN	4096

#define CMD_RELOAD_BOOKMARKS	"reload-bookmarks"

CPipeCommManager::CPipeCommManager()
{
	//默认启动时就自带一条加载书签的命令
	m_qGUICommand.push(CMD_RELOAD_BOOKMARKS);
}

CPipeCommManager::~CPipeCommManager()
{

}

void CPipeCommManager::Run()
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hCreatePipeEvent = NULL;
	HANDLE hDisconnectPipeEvent = NULL;

	do
	{
		hPipe = CreateNamedPipe(
			PIPE_NAME_BOOKMARK_TRANS,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
			1, 0, 0, 0, nullptr);
		if (INVALID_HANDLE_VALUE == hPipe)
		{
			break;
		}

		//1、通知Daemon管道创建好，可以接受书签内容了
		hCreatePipeEvent = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			EVENT_NAME_CONNECT_PIPE
		);
		if (hCreatePipeEvent == NULL)
		{
			break;
		}

		SetEvent(hCreatePipeEvent);

		BOOL connected = ConnectNamedPipe(hPipe, NULL);
		if (!connected) {
			DWORD err = GetLastError();

			if (err == ERROR_PIPE_CONNECTED) {
				// 客户端已经提前连上了，这是正常情况
			}
			else {
				printf("ConnectNamedPipe failed: %d\n", err);
				break;
			}
		}

		while (true)
		{
			// todo：确定收发命令的格式
			std::string sCommand;
			//2、如果从GUI的操作界面，有发送过来的要执行的命令
			if (WaitForCommandFromGUI(sCommand))
			{
				//todo:后面如果是多线程，则要锁管道
				//命令推送到管道
				PushCommandIntoPipe(hPipe, sCommand);
			}

			////4、等待接收Daemon的响应数据
			//todo:GetLastError 109
			DWORD readLen = 0;
			uint64_t totalLen = 0;
			BOOL bRet = ReadFile(hPipe, &totalLen, sizeof(totalLen), &readLen, NULL);
			if (!bRet || readLen == 0)
			{
				continue;
			}
			m_spBookMarks.reset(new char[totalLen + 1]());

			uint64_t recvLen = 0;
			while (recvLen < totalLen)
			{
				int toReadLen = PIPE_READ_LEN;
				//todo:最后一次读取这里缓冲区越界，缓冲区不足4096但硬要读4096
				if (totalLen - recvLen < PIPE_READ_LEN)
				{
					toReadLen = totalLen - recvLen;
				}
				BOOL bRet = ReadFile(hPipe, m_spBookMarks.get() + recvLen, toReadLen, &readLen, nullptr);
				if (!bRet || readLen == 0)
				{
					DWORD dwErr = GetLastError();
					continue;
				}

				recvLen += readLen;
			}
			m_spBookMarks[totalLen] = 0;

			hDisconnectPipeEvent = CreateEvent(
				NULL,
				FALSE,
				FALSE,
				EVENT_NAME_DISCONNECT_PIPE
			);
			if (hDisconnectPipeEvent == NULL)
			{
				break;
			}
			SetEvent(hDisconnectPipeEvent);
		}

		CloseHandle(hPipe);
	} while (0);

	if (hCreatePipeEvent)
	{
		CloseHandle(hCreatePipeEvent);
	}

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
	}
}

BOOL CPipeCommManager::WaitForCommandFromGUI(std::string& sCommand)
{
	//todo:这里等待，后续修改等待方式
	//todo:后续考虑GUI如何把命令塞入queue
	while (true)
	{
		if (IsGUICommandQueueEmpty())
		{
			Sleep(3 * 1000);
			continue;
		}
		break;
	}
	
	//todo：等待到数据
	return PopGUICommandQueue(sCommand);
}

BOOL CPipeCommManager::PushCommandIntoPipe(HANDLE hPipe, const std::string& sCommand)
{
	//todo：后续这里封装和构造发送命令格式
	return WriteFile(hPipe, sCommand.c_str(), MAX_CMD_LEN, NULL, NULL);
}

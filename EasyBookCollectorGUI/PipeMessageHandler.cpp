#include "PipeMessageHandler.h"
#include "framework.h"
#include "ListViewMgr.h"
#include "./json-develop/single_include/nlohmann/json.hpp"
#include <iostream>
#include "BookMarksNode.h"
#include "publicfuncs.h"

using json = nlohmann::json;

#define MAX_CMD_LEN	256
#define PIPE_READ_LEN	4096

CPipeMessageHandler::CPipeMessageHandler():
	m_uBookMarksLen(0)
	//m_hDisconnectPipeEvent(NULL)
{
	//默认启动时就自带一条加载书签的命令
	m_qGUICommand.push(STRING_RELOAD_BOOKMARKS);
	//m_spBookMarksMgr = std::make_shared<CBookMarksMgr>();
	m_mCmdUid[STRING_ADD_ACTIVE_TAB] = UID_ADD_ACTIVE_TAB;
	m_mCmdUid[STRING_RELOAD_BOOKMARKS] = UID_RELOAD_BOOKMARKS;
	m_mCmdUid[STRING_RELOAD_BOOKMARKS_WITH_ID] = UID_RELOAD_BOOKMARKS_WITH_ID;
	m_mCmdUid[STRING_ADD_BOOKMARK] = UID_ADD_BOOKMARK;
}

CPipeMessageHandler::~CPipeMessageHandler()
{
}

void CPipeMessageHandler::Run()
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	HANDLE hCreatePipeEvent = NULL;
	HANDLE hCommandFinishedEvent = NULL;//命令执行完成的Event

	hPipe = CreateNamedPipe(
		PIPE_NAME_BOOKMARK_TRANS,
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, //PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,//PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1, 0, 0, 0, nullptr);
	if (INVALID_HANDLE_VALUE == hPipe)
	{
		return;
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
		return;
	}
	//todo：这个的位置是不是放到ConnectNamedPipe后面？
	SetEvent(hCreatePipeEvent);

	BOOL connected = ConnectNamedPipe(hPipe, NULL);
	if (!connected) 
	{
		DWORD err = GetLastError();

		if (err == ERROR_PIPE_CONNECTED) 
		{
			// 客户端已经提前连上了，这是正常情况
		}
		else 
		{
			printf("ConnectNamedPipe failed: %d\n", err);
			return;
		}
	}

	do
	{
		//集中到一个地方接收命令并派发命令给NativeMessageDemo.exe
		while (true)
		{
			// todo：确定收发命令的格式
			std::string sCommand;
			//如果从GUI的操作界面，有发送过来的要执行的命令
			//否则这里会循环等待
			//todo：这里将来在队列中加个时间戳排序，按时间戳的顺序取出，所以不能再以一个单独的字符串作为命令行了，而应该以一个package
			//这里暂时还没有多线程去取命令执行，所以这里的循环保证每个命令的处理顺序和每个命令的发送过来的顺序是一致的
			//如果是多线程的，则必须把“待插入的目录、发送命令到GUI、网络传回书签”这三者原子化，才能绑定“待插入目录”和书签的关系
			if (WaitForCommandFromGUI(sCommand))
			{
				//todo:后面如果是多线程，则要锁管道
				//命令推送到管道
				WriteCommandIntoPipe(hPipe, sCommand);
			}
			//todo：连续几次之后这里发送获取书签命令但是没有拿到书签数据
			switch (ConvertCmdToUid(sCommand))
			{
			case UID_ADD_ACTIVE_TAB:
			{
				//这里的循环确保了每次网络操作操作完成之后才去读下一次命令
				HandleActiveTabInfo(hPipe);
				//todo：获得新的书签，插入书签然后重新Reload或者Reparse
				//todo：这里使用待插入的文件夹信息，并刷新VirtualFolder
				break;
			}
			case UID_RELOAD_BOOKMARKS:
			{
				HandleBookmarksData(hPipe);
				break;
			}
			case UID_RELOAD_BOOKMARKS_WITH_ID:
			{
				HandleBookmarksDataWithId(hPipe);
				break;
			}
			case UID_ADD_BOOKMARK:
			{
				//todo：等待添加结果的返回值，拿到返回的bookmark id
				break;
			}
			case UID_DISCONNECT:
			{
				//这里发送断开链接的命令
				Disconnect();//这里不break还得执行下边的清理操作
			}
			}

			//这里是命令执行完成的通知,通知守护进程可以去取下一个命令了
			hCommandFinishedEvent = CreateEvent(
				NULL,
				FALSE,
				FALSE,
				EVENT_NAME_CMD_FINISHED
			);
			if (hCommandFinishedEvent == NULL)
			{
				break;
			}
			SetEvent(hCommandFinishedEvent);
		}

	} while (0);
	
	if (hCommandFinishedEvent)
	{
		CloseHandle(hCommandFinishedEvent);
	}

	if (hCreatePipeEvent)
	{
		CloseHandle(hCreatePipeEvent);
	}

	if (hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
	}
}
////todo：这一步为什么要封装
//std::vector<CBookMarksNode>& CPipeMessageHandler::GetAllBookMarksNodes()
//{
//	return m_spBookMarksMgr->GetAllBookMarksNodes();
//}
//
//uint64_t CPipeMessageHandler::GetBookMarksCnt()
//{
//	return m_spBookMarksMgr->GetBookMarksCnt();
//}
//
//std::shared_ptr<CBookMarksMgr>& CPipeMessageHandler::GetBookMarksMgrPointer()
//{
//	return m_spBookMarksMgr;
//}

//VOID CPipeMessageHandler::InsertBookMarkNode(const CBookMarksMgr& node) noexcept
//{
//
//}

VOID CPipeMessageHandler::PushGUICommandToQueue(const std::string& data)
{
	std::lock_guard<std::mutex> lock(m_mtxCmdQueue);
	m_qGUICommand.push(data);
}

BOOL CPipeMessageHandler::WaitForCommandFromGUI(std::string& sCommand)
{
	while (true)
	{
		//todo:这里等待，后续修改等待方式
		if (IsGUICommandQueueEmpty())
		{
			Sleep(3 * 1000);
			continue;
		}
		break;
	}
	
	//todo：等待到数据
	return GetGUICommandFromQueue(sCommand);
}

BOOL CPipeMessageHandler::WriteCommandIntoPipe(HANDLE hPipe, const std::string& sCommand)
{
	//todo：后续这里封装和构造发送命令格式
	return WriteFile(hPipe, sCommand.c_str(), MAX_CMD_LEN, NULL, NULL);
}

BOOL CPipeMessageHandler::GetGUICommandFromQueue(std::string& out)
{
	std::lock_guard<std::mutex> lock(m_mtxCmdQueue);
	if (m_qGUICommand.empty())
	{
		return false;
	}
	out = m_qGUICommand.front();
	m_qGUICommand.pop();
	return true;
}

BOOL CPipeMessageHandler::IsGUICommandQueueEmpty()
{
	std::lock_guard<std::mutex> lock(m_mtxCmdQueue);
	return m_qGUICommand.empty();
}

VOID CPipeMessageHandler::DumpToFile(const char* data, int length, std::string_view svDumpPath)
{
	FILE* fp = nullptr;

	// 安全打开文件，wb = 二进制写入
	errno_t err = fopen_s(&fp, svDumpPath.data(), "wb");

	if (err != 0 || fp == nullptr)
	{
		printf("打开文件失败\n");
		return;
	}

	// 写入完整数据
	fwrite(data, 1, length, fp);

	// 关闭文件
	fclose(fp);
}

//todo：这里把读取操作抽象出来
BOOL CPipeMessageHandler::HandleActiveTabInfo(HANDLE hPipe)
{
	BOOL bRet = FALSE;
	do 
	{
		//等待接收Daemon的响应数据
		//todo：这里是否设置等待超时？不然某一次卡住怎么办
		if (!ReadDataFromPipe(hPipe, m_spActiveTabInfo, m_uActiveTabInfoLen))
		{
			break;
		}

		DumpToFile(m_spActiveTabInfo.get(), m_uActiveTabInfoLen, "activetabinfo_dump.bin");

		std::optional<std::pair<std::string, std::string>> activeInfo = ParseActiveInfo();
		if (!activeInfo.has_value())
		{
			break;
		}

		//拿到待插入到的目录信息
		std::optional<CBookMarksNode> InsertedFolder = CListViewMgr::instance().GetInsertedFolder();
		if (!InsertedFolder.has_value())
		{
			break;
		}
		// 这里是增加到vector，然后将来再点击文件夹进入的时候会自动借助以前的VisitSubListViewFolder来刷新
		CBookMarksMgr::instance().InsertBookInfoUnderFolder(PublicFuncs::UTF8ToWString(activeInfo->first.c_str(), activeInfo->first.length()),
			PublicFuncs::UTF8ToWString(activeInfo->second.c_str(), activeInfo->first.length()), InsertedFolder->GetId());

		CListViewMgr::instance().RefreshCurrentListView(InsertedFolder->GetId());
		//m_qGUICommand.push(STRING_RELOAD_BOOKMARKS_WITH_ID);
		m_qGUICommand.push(STRING_ADD_BOOKMARK);
		
		bRet = TRUE;
	} while (0);
	
	return bRet;
}

BOOL CPipeMessageHandler::HandleBookmarksData(HANDLE hPipe)
{
	if (!ReadDataFromPipe(hPipe, m_spBookMarksData, m_uBookMarksLen))
	{
		return FALSE;
	}
	DumpToFile(m_spBookMarksData.get(), m_uBookMarksLen, "bookmarkes_dump.bin");//todo:构建树形结构
	ParseToBookmarkTree();
	//todo：本地插入方案：1、重新reload；2、不重新reload
	// todo：本地刷新或者重载？
	//todo：发送通知到Firefox：是一个一个提交还是多个操作之后一起提交？
	
	return TRUE;
}

BOOL CPipeMessageHandler::HandleBookmarksDataWithId(HANDLE hPipe)
{
	if (!ReadDataFromPipe(hPipe, m_spBookMarksData, m_uBookMarksLen))
	{
		return FALSE;
	}
	DumpToFile(m_spBookMarksData.get(), m_uBookMarksLen, "bookmarkes_dump_id.bin");
	//ParseToBookmarkTree();

	return TRUE;
}

BOOL CPipeMessageHandler::AddBookMark(HANDLE hPipe)
{
	return TRUE;
}

//todo：如果是一个空文件夹，不会传递过来
//
VOID CPipeMessageHandler::ParseToBookmarkTree()
{
	uint64_t start = 0;
	uint64_t end = 0;
	BOOL bPrasingFolder = TRUE;
	BOOL bParsingDescription = FALSE;
	BOOL bParsingWebsite = FALSE;
	std::wstring sLastFolderName;
	std::wstring sWebsiteName;
	std::wstring sWebsiteDes;
	std::wstring sFolderName;

	uint64_t tmpcount = 0;
	//解析基于如下事实：
	//1、每条存储信息占一行，由换行键决定
	//2、文件夹的名称没有/存在 todo：当然我们可以测试下有/存在的文件夹Firefox是怎么处理的
	//3、同一目录下必须是连续出现的
	if (m_uBookMarksLen < 1000)
	{
		printf("%s", m_spBookMarksData.get());
	}
	for (int i = 0; i < m_uBookMarksLen; i++)
	{
		if (bPrasingFolder == TRUE)
		{
			if (m_spBookMarksData[i] == '[')
			{
				start = i + 1;
			}

			if (m_spBookMarksData[i] == ']')
			{
				end = i;
				sFolderName = PublicFuncs::UTF8ToWString(m_spBookMarksData.get() + start, end - start);
				if (sFolderName != sLastFolderName)//如果本次解析的文件夹路径和上次一样就不解析了
				{
					CBookMarksMgr::instance().InsertFolder(sFolderName);
				}
				sLastFolderName = sFolderName;
				start = i + 1;
				bPrasingFolder = FALSE;
				bParsingDescription = TRUE;
				continue;
			}
		}

		if (bParsingDescription == TRUE)
		{
			if (m_spBookMarksData[i] == '=' && m_spBookMarksData[i + 1] == '>')
			{
				end = i - 1;
				sWebsiteName = PublicFuncs::Trim(PublicFuncs::UTF8ToWString(m_spBookMarksData.get() + start, end - start));
				start = i + 2;
				bParsingWebsite = TRUE;
				bParsingDescription = FALSE;
				continue;
			}
		}

		if (bParsingWebsite == TRUE)
		{
			if (m_spBookMarksData[i] == '\n' || i == m_uBookMarksLen - 1)//最后一行没有换行符
			{
				end = i;
				uint64_t length = end - start;
				if (i == m_uBookMarksLen - 1)//单独处理文本的最后一行，因为最后一行没有换行符
				{
					length = end - start + 1;
				}
				sWebsiteDes = PublicFuncs::Trim(PublicFuncs::UTF8ToWString(m_spBookMarksData.get() + start, length));
				CBookMarksMgr::instance().InsertBookInfoUnderFolder(sWebsiteName, sWebsiteDes, CBookMarksMgr::instance().GetCurrentNode().GetId());//todo：这里调用关系这么长？
				bParsingWebsite = FALSE;
				bPrasingFolder = TRUE;
				//todo：分析手动改变FireFox中书签的顺序是否有影响
				//todo：如果GUI崩溃了，再启动GUI是否可以直接连接上Native
				//todo：目前还有一个bug，就是如果我不关闭Native，连续几次启动关闭GUI.exe，就会有出现加载文件夹不完全的情况出现
			}
		}
	}

	HANDLE hLoadedBookmarksEvent = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		EVENT_NAME_LOADED_BOOKMARKS
	);

	if (hLoadedBookmarksEvent == NULL)
	{
		//todo：如何进行错误处理？
		return;
	}

	SetEvent(hLoadedBookmarksEvent);
}

std::optional<std::pair<std::string, std::string>> CPipeMessageHandler::ParseActiveInfo()
{
	std::optional<std::pair<std::string, std::string>> activeInfo;
	if (!m_spActiveTabInfo)
	{
		return std::nullopt;;
	}

	std::string sActiveInfo = m_spActiveTabInfo.get();
	try {
		json j = json::parse(sActiveInfo);
		activeInfo = std::make_pair(j.at("data").at("title").get<std::string>(), j.at("data").at("url").get<std::string>());
	}
	catch (json::exception& e)
	{
		std::cerr << "JSON error: " << e.what() << std::endl;
		std::string errMsg = std::string("JSON error: ") + e.what();
		activeInfo = std::nullopt;;
	}
	
	return activeInfo;
}

BOOL CPipeMessageHandler::Disconnect()
{
	/*m_hDisconnectPipeEvent = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		EVENT_NAME_DISCONNECT_PIPE
	);
	if (m_hDisconnectPipeEvent == NULL)
	{
		return FALSE;
	}
	SetEvent(m_hDisconnectPipeEvent);*/

	return TRUE;
}

//std::wstring CPipeCommManager::UTF8ToWString(const char* utf8, int length)
//{
//	if (!utf8 || length <= 0)
//		return {};
//
//	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, length, nullptr, 0);
//	std::wstring result(wlen, L'\0');
//	MultiByteToWideChar(CP_UTF8, 0, utf8, length, &result[0], wlen);
//	return result;
//}

UINT CPipeMessageHandler::ConvertCmdToUid(std::string_view command)
{
	auto it = m_mCmdUid.find(command);
	if (it == m_mCmdUid.end())
	{
		return 0;
	}

	return it->second;
}

BOOL CPipeMessageHandler::ReadDataFromPipe(HANDLE hPipe, std::shared_ptr<char[]>& spData, uint64_t& uTotalDataLen)
{
	DWORD readLen = 0;
	DWORD dwTotalLen = 0;
	BOOL bRet = ReadFile(hPipe, &uTotalDataLen, sizeof(uTotalDataLen), &readLen, NULL);
	if (!bRet || readLen == 0)//读长度一次就能读完
	{
		return FALSE;
	}
	spData.reset(new char[uTotalDataLen + 1]());

	uint64_t recvLen = 0;
	while (recvLen < uTotalDataLen)
	{
		int toReadLen = PIPE_READ_LEN;
		if (uTotalDataLen - recvLen < PIPE_READ_LEN)
		{
			toReadLen = uTotalDataLen - recvLen;
		}
		//用了 消息模式（MESSAGE）消息模式规定：一条消息可能分多次读完只要没读完ReadFile 返回 FALSE
		BOOL bRet = ReadFile(hPipe, spData.get() + recvLen, toReadLen, &readLen, nullptr);
		if (!bRet && readLen > 0)
		{
			//这里只是为了消除C28193警告
		}
		if (readLen == 0)
		{
			DWORD dwErr = GetLastError();
			continue;
		}

		recvLen += readLen;
	}
	spData[uTotalDataLen] = 0;

	return TRUE;
}
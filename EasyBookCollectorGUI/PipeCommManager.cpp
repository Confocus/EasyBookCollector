#include "PipeCommManager.h"
#include "framework.h"


#define MAX_CMD_LEN	256
#define PIPE_READ_LEN	4096

uint64_t g_uBookMarkNodeId = 0;
CPipeCommManager::CPipeCommManager():
	m_uTotalLen(0)
	//m_hDisconnectPipeEvent(NULL)
{
	//默认启动时就自带一条加载书签的命令
	m_qGUICommand.push(STRING_RELOAD_BOOKMARKS);
	m_spBookMarksMgr = std::make_shared<BookMarksMgr>();
	m_mCmdUid[STRING_ADD_ACTIVE_TAB] = UID_ADD_ACTIVE_TAB;
	m_mCmdUid[STRING_RELOAD_BOOKMARKS] = UID_RELOAD_BOOKMARKS;
}

CPipeCommManager::~CPipeCommManager()
{
}

void CPipeCommManager::Run()
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
			if (WaitForCommandFromGUI(sCommand))
			{
				//todo:后面如果是多线程，则要锁管道
				//命令推送到管道
				WriteCommandIntoPipe(hPipe, sCommand);
			}
			//todo：连续几次之后这里发送获取书签命令但是没有拿到书签数据
			
			
			//等待接收Daemon的响应数据
			DWORD readLen = 0;
			
			BOOL bRet = ReadFile(hPipe, &m_uTotalLen, sizeof(m_uTotalLen), &readLen, NULL);
			if (!bRet || readLen == 0)//读长度一次就能读完
			{
				continue;
			}
			m_spBookMarksData.reset(new char[m_uTotalLen + 1]());

			uint64_t recvLen = 0;
			while (recvLen < m_uTotalLen)
			{
				int toReadLen = PIPE_READ_LEN;
				if (m_uTotalLen - recvLen < PIPE_READ_LEN)
				{
					toReadLen = m_uTotalLen - recvLen;
				}
				//用了 消息模式（MESSAGE）消息模式规定：一条消息可能分多次读完只要没读完ReadFile 返回 FALSE
				BOOL bRet = ReadFile(hPipe, m_spBookMarksData.get() + recvLen, toReadLen, &readLen, nullptr);
				if (readLen == 0)
				{
					DWORD dwErr = GetLastError();
					continue;
				}

				recvLen += readLen;
			}
			m_spBookMarksData[m_uTotalLen] = 0;
			DumpToFile(m_spBookMarksData.get(), m_uTotalLen);//todo:构建树形结构

			switch (ConvertCmdToUid(sCommand))
			{
				//todo：获取当前激活的Tab的页面信息
			case UID_ADD_ACTIVE_TAB:
			{
				//todo：获得新的书签，插入书签然后重新Reload或者Reparse
				break;
			}
			case UID_RELOAD_BOOKMARKS:
			{
				ParseToBookmarkTree();
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

std::vector<BookMarksNode>& CPipeCommManager::GetAllBookMarksNodes()
{
	return m_spBookMarksMgr->GetAllBookMarksNodes();
}

uint64_t CPipeCommManager::GetBookMarksCnt()
{
	return m_spBookMarksMgr->GetBookMarksCnt();
}

std::shared_ptr<BookMarksMgr>& CPipeCommManager::GetBookMarksMgrPointer()
{
	return m_spBookMarksMgr;
}

BOOL CPipeCommManager::WaitForCommandFromGUI(std::string& sCommand)
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

BOOL CPipeCommManager::WriteCommandIntoPipe(HANDLE hPipe, const std::string& sCommand)
{
	//todo：后续这里封装和构造发送命令格式
	return WriteFile(hPipe, sCommand.c_str(), MAX_CMD_LEN, NULL, NULL);
}

//todo：如果是一个空文件夹，不会传递过来
//
VOID CPipeCommManager::ParseToBookmarkTree()
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
	if (m_uTotalLen < 1000)
	{
		printf("%s", m_spBookMarksData.get());
	}
	for (int i = 0; i < m_uTotalLen; i++)
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
				sFolderName = UTF8ToWString(m_spBookMarksData.get() + start, end - start);
				if (sFolderName != sLastFolderName)
				{
					m_spBookMarksMgr->InsertFolder(sFolderName);
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
				sWebsiteName = Trim(UTF8ToWString(m_spBookMarksData.get() + start, end - start));
				start = i + 2;
				bParsingWebsite = TRUE;
				bParsingDescription = FALSE;
				continue;
			}
		}

		if (bParsingWebsite == TRUE)
		{
			if (m_spBookMarksData[i] == '\n' || i == m_uTotalLen - 1)//最后一行没有换行符
			{
				end = i;
				uint64_t length = end - start;
				if (i == m_uTotalLen - 1)//单独处理文本的最后一行，因为最后一行没有换行符
				{
					length = end - start + 1;
				}
				sWebsiteDes = Trim(UTF8ToWString(m_spBookMarksData.get() + start, length));
				m_spBookMarksMgr->InsertBookInfoUnderFolder(sWebsiteName, sWebsiteDes);
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

BOOL CPipeCommManager::Disconnect()
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

BookMarksMgr::BookMarksMgr():
	m_nLastFatherNum(-1)
{

}

BookMarksMgr::~BookMarksMgr()
{

}

//不过这里基于一个事实，就是[]是排好序的
//[书签菜单] 1
//[书签菜单 / 2026 / google时政] 1 2 3
//这里记住上一次的是1 2 3
//[书签菜单 / 2026 / IT2026 / 安全咨询]1 2 4 5
//
//这里记录上次是1 2 4 5
//这里第一个"书签工具栏"就不匹配。匹配到哪里就从哪里继续插入
//[书签工具栏 / 书籍 / 20190802]6 7 8
//
//{
//	// 根节点（parent_id=-1）
//	{1, true, L"我的图书分类", -1, 0, L""},
//	{ 2, true, L"我的收藏夹", -1, 0, L"" },
//	{ 3, false, L"临时笔记.txt", -1, 1001, L"这是自定义数据项，不是文件" },
//		// 图书分类的子节点（parent_id=1）
//	{ 4, true, L"编程类", 1, 0, L"" },
//	{ 5, true, L"小说类", 1, 0, L"" },
//	{ 6, false, L"Python实战.md", 2, 1002, L"Python入门教程，自定义数据" },
//		// 编程类的子节点（parent_id=4）
//	{ 7, false, L"Java核心技术.md", 4, 1003, L"Java进阶内容，自定义数据" },
//	{ 8, false, L"C++ Primer.md", 4, 1004, L"C++基础，自定义数据" },
//	{ 9, false, L"D++ Primer.md", 4, 1004, L"C++基础，自定义数据" },
//	{ 10, false, L"E++ Primer.md", 4, 1004, L"C++基础，自定义数据" },
//	{ 11, false, L"F++ Primer.md", 4, 1004, L"C++基础，自定义数据" },
//	{ 12, false, L"G++ Primer.md", 4, 1004, L"C++基础，自定义数据" },
//	{ 13, false, L"H++ Primer.md", 4, 1004, L"C++基础，自定义数据" },
//};
VOID BookMarksMgr::InsertFolder(const std::wstring s)
{
	size_t start = 0;
	size_t end = s.find(L'/');

	// 循环切割
	uint64_t uNum = 1;
	std::vector<std::wstring> vecFolders;

	//拆分目录例如“书签菜单/理财/股票”保存进vector
 	while (end != std::wstring::npos)
	{
		std::wstring sFolderName = s.substr(start, end - start);
		// 下一段
		start = end + 1;
		end = s.find(L'/', start);
		vecFolders.push_back(sFolderName);
	}

	// 最后一段
	if (start < s.size())
	{
		// 截取一段
		vecFolders.push_back(s.substr(start, end - start));
	}

	//建立在ListView中显示的文件夹的父子关系
	uint64_t uSameNodeCnt = 0;
	//寻找最短的公共路径
	for (auto i = 0; i < vecFolders.size(); i++)
	{
		if (i < min(vecFolders.size(), m_vecLastFolders.size()) && vecFolders[i] == m_vecLastFolders[i])
		{
			uSameNodeCnt++;
			continue;
		}
		break;
	}

	//如果两个vector有公共的文件夹
	if (uSameNodeCnt != 0)
	{
		uint64_t uPopCnt = m_vecLastFolders.size() - uSameNodeCnt;
		for (int i = 0; i < uPopCnt; i++)
		{
			if (!m_vecLastNodes.empty())
			{
				m_vecLastNodes.pop_back();
			}
		}
		BookMarksNode LastNode;
		if (!m_vecLastFolders.empty())
		{
			LastNode = m_vecLastNodes.back();
			m_nLastFatherNum = LastNode.m_uNum;
			//避免下一次遍历的目录比上一次遍历的目录短的情况出现，比如：
			//[书签菜单/2026/IT2026/前沿科学]
			//[书签菜单/2026/IT2026]
			m_uCurrentNode = LastNode;
		}
	}
	else//说明没有公共路径，那就重新以根节点为根目录
	{
		m_nLastFatherNum = -1;
	}
	
	for (auto i = 0; i < vecFolders.size(); i++)
	{
		if(i < min(vecFolders.size(), m_vecLastFolders.size()) && vecFolders[i] == m_vecLastFolders[i])
		{
			continue;
		}
		
		BookMarksNode node;
		// 截取一段
		node.m_bIsFolder = true;
		node.m_nFatherNum = m_nLastFatherNum;
		//node.m_nFatherNum = LastNode.m_nFatherNum;
		node.m_uNum = m_vecNodes.size() + 1;//计数从1开始
		m_nLastFatherNum = node.m_uNum;
		node.m_sName = vecFolders[i];
		node.m_uId = g_uBookMarkNodeId++;
		m_vecNodes.push_back(node);
		m_uCurrentNode = node;
		m_vecLastNodes.push_back(node);
	}
	m_vecLastFolders = vecFolders;
}
//todo：有一种情况下会出错，就是上一次的NativeMessage.exe没结束掉，然后重启GUI.exe，然后再结束掉Native.exe再重启Native.exe在重载Firefox
// todo：为什么如果不关闭Native.exe，每次启动GUI。exe就可以自动获取到数据？
// 
//ItemNode g_szTestNode[] =
//{
//	// 根节点（parent_id=-1）
//	{1, true, L"我的图书分类", -1, 0, L""},
//	{2, true, L"我的收藏夹", -1, 0, L""},
//	{3, false, L"临时笔记.txt", -1, 1001, L"这是自定义数据项，不是文件"},
//	// 图书分类的子节点（parent_id=1）
//	{4, true, L"编程类", 1, 0, L""},
//	{5, true, L"小说类", 1, 0, L""},
//	{6, false, L"Python实战.md", 2, 1002, L"Python入门教程，自定义数据"},
//	// 编程类的子节点（parent_id=4）
//	{7, false, L"Java核心技术.md", 4, 1003, L"Java进阶内容，自定义数据"},
//	{8, false, L"C++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
//	{9, false, L"D++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
//	{10, false, L"E++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
//	{11, false, L"F++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
//	{12, false, L"G++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
//	{13, false, L"H++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
//};
VOID BookMarksMgr::InsertBookInfoUnderFolder(const std::wstring name, const std::wstring des)
{
	BookMarksNode node;
	// 截取一段
	node.m_bIsFolder = false;
	node.m_nFatherNum = m_uCurrentNode.m_uNum;
	node.m_uNum = m_vecNodes.size() + 1;//计数从1开始
	node.m_sName = name;
	node.m_sDescription = des;
	node.m_uId = g_uBookMarkNodeId++;//这玩意儿有用吗？通过uId查找节点的时候需要一个唯一编号
	m_vecNodes.push_back(node);
}

std::vector<BookMarksNode>& BookMarksMgr::GetAllBookMarksNodes()
{
	return m_vecNodes;
}

uint64_t BookMarksMgr::GetBookMarksCnt()
{
	return m_vecNodes.size();
}

std::optional<BookMarksNode> BookMarksMgr::FindIndexById(uint64_t uid)
{
	auto it = find_if(m_vecNodes.begin(), m_vecNodes.end(), [uid](const BookMarksNode& item) {
		return item.m_uId == uid;
		});

	if (it == m_vecNodes.end())
		return std::nullopt; // 没找到

	return *it;
}

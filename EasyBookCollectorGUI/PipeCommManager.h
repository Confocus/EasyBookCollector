#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <tchar.h>
#include <thread>
#include "MainWindowActions.h"
#include <shellapi.h>
#include <array>
#include <queue>
#include <mutex>
#include <map>

class BookMarksNode
{
public:
	BookMarksNode():
		m_bIsFolder(TRUE),
		m_uNum(0),
		m_nFatherNum(-1),
		m_uId(0)
	{

	}
	~BookMarksNode()
	{

	}
public:
	BOOL m_bIsFolder;
	uint64_t m_uNum;
	int64_t m_nFatherNum;
	/*int64_t m_nSonNum;
	int64_t m_nSiblingNum;*/
	//int64_t m_nLevelNum;
	uint64_t m_uId;
	std::wstring m_sDescription;
	std::wstring m_sName;
};

class BookMarksMgr
{
public:
	BookMarksMgr();
	~BookMarksMgr();
	VOID InsertFolder(const std::wstring);
	VOID InsertBookInfoUnderFolder(const std::wstring, const std::wstring);
	std::vector<BookMarksNode>& GetAllBookMarksNodes();
	uint64_t GetBookMarksCnt();
	std::optional<BookMarksNode> FindIndexById(uint64_t uid);
private:
	//int64_t m_uCurrentPointer;//现在遍历到哪个目录了，方便直接插入数据
	BookMarksNode m_uCurrentNode;
	std::vector<BookMarksNode> m_vecNodes;//每一个文件夹或文件都被当做一个Node保存到了这个数组里
	std::vector<BookMarksNode> m_vecLastNodes;
	std::vector<std::wstring> m_vecLastFolders;//保存上一次操作的文件夹路径序列，便于判断下一次从哪开始插入
	int64_t m_nLastFatherNum;
	//std::wstring sFolderName;//文件夹的名字、自己的名字
	//std::vector<std::wstring> vecBooks;
	//std::vector<std::shared_ptr<BookMarksTree*>> vecFolders;
};

class CPipeCommManager
{
public:
	CPipeCommManager();
	~CPipeCommManager();

	void Run();
	std::vector<BookMarksNode>& GetAllBookMarksNodes();
	uint64_t GetBookMarksCnt();
	std::shared_ptr<BookMarksMgr>& GetBookMarksMgrPointer();

	VOID PushGUICommandToQueue(const std::string& data);
	
private:
	BOOL WaitForCommandFromGUI(std::string& sCommand);

	//把命令写进管道
	BOOL WriteCommandIntoPipe(HANDLE hPipe, const std::string& sCommand);
	
	//把管道中的命令拿出来
	BOOL GetGUICommandFromQueue(std::string& out);
	
	BOOL IsGUICommandQueueEmpty();

	//todo:后续改成单独一个线程出来缓存到本地
	VOID DumpToFile(const char* data, int length);
	
	BOOL ReadActiveTabInfoFromPipe(HANDLE hPipe);
	BOOL ReadBookMarksFromPipeAndParse(HANDLE hPipe);
	VOID ParseToBookmarkTree();
	BOOL Disconnect();

	std::wstring UTF8ToWString(const char* utf8, int length);

	std::wstring Trim(std::wstring_view s);
	
	//命令行转成id供switch case判断
	UINT ConvertCmdToUid(std::string_view command);

	//直接从管道读取数据的代码
	BOOL ReadDataFromPipe(HANDLE hPipe, std::shared_ptr<char[]>& spData, uint64_t& uTotalDataLen);

private:
	//HANDLE m_hDisconnectPipeEvent;
	std::queue<std::string> m_qGUICommand;
	std::mutex m_mutex;
	std::shared_ptr<char[]> m_spActiveTabInfo;
	uint64_t m_uActiveTabInfoLen;
	std::shared_ptr<char[]> m_spBookMarksData;
	uint64_t m_uBookMarksLen;
	std::shared_ptr<BookMarksMgr> m_spBookMarksMgr;
	//保存从命令行string到uid的转换
	std::map<std::string_view, unsigned int> m_mCmdUid;
};
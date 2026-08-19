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
#include "BookMarksNode.h"
#include "CBookMarksMgr.h"
/**
 * @brief 通信组建
 * @details 分发命令到守护进程，接收从守护进程发过来的数据
 */
class CPipeCommManager
{
public:
	CPipeCommManager();
	~CPipeCommManager();

	/**
	* @brief 建立管道同守护进程进行通信。这里会中转命令。
	* @param parentFolder 待插入到的目标父目录节点
	*/
	void Run();
	std::vector<CBookMarksNode>& GetAllBookMarksNodes();
	uint64_t GetBookMarksCnt();
	std::shared_ptr<BookMarksMgr>& GetBookMarksMgrPointer();
	VOID InsertBookMarkNode(const BookMarksMgr& node) noexcept;
	VOID PushGUICommandToQueue(const std::string& data);
	
private:
	/**
	* @brief 等待从其它线程传递过来命令在
	* @param 命令参数
	* @return 成功返回true，失败返回false
	*/
	BOOL WaitForCommandFromGUI(std::string& sCommand);

	//把命令写进管道
	BOOL WriteCommandIntoPipe(HANDLE hPipe, const std::string& sCommand);
	
	//把管道中的命令拿出来
	BOOL GetGUICommandFromQueue(std::string& out);
	
	BOOL IsGUICommandQueueEmpty();

	//todo:后续改成单独一个线程出来缓存到本地
	VOID DumpToFile(const char* data, int length, std::string_view svDumpPath);//todo:构建树形结构
	
	BOOL HandleActiveTabInfo(HANDLE hPipe);
	BOOL HandleBookmarksData(HANDLE hPipe);
	VOID ParseToBookmarkTree();
	std::optional<std::pair<std::string, std::string>> ParseActiveInfo();
	BOOL Disconnect();

	//std::wstring UTF8ToWString(const char* utf8, int length);

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
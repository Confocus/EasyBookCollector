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
#include "singleton.h"

/**
 * @brief 通信组建
 * @details 分发命令到守护进程，接收从守护进程发过来的数据
 */
class CPipeMessageHandler : public Singleton<CPipeMessageHandler>
{
	friend Singleton<CPipeMessageHandler>;
public:
	/**
	* @brief 数据通信处理函数。
	* @details 建立管道同守护进程进行通信；完成命令中转、命令接收与解析；Run运行在独立线程，与界面线程隔离。
	* @param parentFolder 待插入到的目标父目录节点
	*/
	void Run();
	VOID PushGUICommandToQueue(const std::string& data);

private:
	CPipeMessageHandler();
	~CPipeMessageHandler();

	CPipeMessageHandler(const CPipeMessageHandler&) = delete;
	CPipeMessageHandler& operator=(const CPipeMessageHandler&) = delete;

	// 删除移动构造、移动赋值
	CPipeMessageHandler(CPipeMessageHandler&&) = delete;
	CPipeMessageHandler& operator=(CPipeMessageHandler&&) = delete;
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
	
	/**
	* @brief 获取当前活跃的页面信息。
	* @details 主要是为了“添加”该页面到书签中
	* @param hPipe 与后台进程通信的管道句柄
	*/
	BOOL HandleActiveTabInfo(HANDLE hPipe);

	/**
	* @brief 获取全部书签信息。
	* @details 
	* @param hPipe 与后台进程通信的管道句柄
	*/
	BOOL HandleBookmarksData(HANDLE hPipe);

	/**
	* @brief 获取全部书签信息，但是携带文件夹id。
	* @details 用来测试获取的文件夹是否能携带唯一id，以便本地添加完文件夹之后能正确同步到浏览器中
	* @param hPipe 与后台进程通信的管道句柄
	*/
	BOOL HandleBookmarksDataWithId(HANDLE hPipe);

	BOOL AddBookMark(HANDLE hPipe);

	/**
	* @brief 解析从浏览器过来的书签数据
	* @details 格式类似于[书签菜单/2026/投研] 研报精选 - 萝卜投研 => https://robo.datayes.com/v2/selection
	*/
	VOID ParseToBookmarkTree();

	/**
	* @brief 解析从浏览器过来的书签数据
	* @details 由于担心后面会获取更详细的书签信息，导致每一行书签信息有所增加，所以使用正则表达式去解析会更灵活一点
	*/
	VOID ParseToBookmarkTreeByRegexp();

	std::optional<std::pair<std::string, std::string>> ParseActiveInfo();
	BOOL Disconnect();

	
	//命令行转成id供switch case判断
	UINT ConvertCmdToUid(std::string_view command);

	//直接从管道读取数据的代码
	BOOL ReadDataFromPipe(HANDLE hPipe, std::shared_ptr<char[]>& spData, uint64_t& uTotalDataLen);

private:
	//HANDLE m_hDisconnectPipeEvent;
	std::queue<std::string> m_qGUICommand;
	mutable std::mutex m_mtxCmdQueue;//操作命令队列的锁
	std::shared_ptr<char[]> m_spActiveTabInfo;
	uint64_t m_uActiveTabInfoLen;
	std::shared_ptr<char[]> m_spBookMarksData;
	uint64_t m_uBookMarksLen;
	//保存从命令行string到uid的转换
	std::map<std::string_view, unsigned int> m_mCmdUid;
};
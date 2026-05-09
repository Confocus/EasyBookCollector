#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <tchar.h>
#include <thread>
#include "EasyBookCollectorGUI.h"
#include "MainWindowActions.h"
#include "ListBoxWndManager.h"
#include <shellapi.h>
#include "ListViewMgr.h"
#include <array>
#include <queue>
#include <mutex>

class BookMarksTreeNode
{
public:
	VOID InsertFolder(const std::string);
	VOID InsertBookInfoUnderFolder(const std::string, const std::string, const std::string);
private:
	

private:
	std::string sFolderName;//文件夹的名字、自己的名字
	std::vector<std::string> vecBooks;
	std::vector<std::shared_ptr<BookMarksTreeNode*>> vecFolders;
}

class CPipeCommManager
{
public:
	CPipeCommManager();
	~CPipeCommManager();

	void Run();
private:
	BOOL WaitForCommandFromGUI(std::string& sCommand);

	BOOL PushCommandIntoPipe(HANDLE hPipe, const std::string& sCommand);
	
	VOID PushGUICommandQueue(const std::string& data)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_qGUICommand.push(data);
	}

	BOOL PopGUICommandQueue(std::string& out)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_qGUICommand.empty())
		{
			return false;
		}
		out = m_qGUICommand.front();
		m_qGUICommand.pop();
		return true;
	}

	BOOL IsGUICommandQueueEmpty()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_qGUICommand.empty();
	}

	//todo:后续改成单独一个线程出来缓存到本地
	VOID DumpToFile(const char* data, int length)
	{
		FILE* fp = nullptr;

		// 安全打开文件，wb = 二进制写入
		errno_t err = fopen_s(&fp, "bookmarkes_dump.bin", "wb");

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

	VOID ParseToBookmarkTree();

	std::string Trim(std::string_view s)
	{
		auto l = s.find_first_not_of(" \t\n\r");
		auto r = s.find_last_not_of(" \t\n\r");
		if (l == s.npos) return {};
		return std::string(s.substr(l, r - l + 1));
	}
private:
	std::queue<std::string> m_qGUICommand;
	std::mutex m_mutex;
	std::shared_ptr<char[]> m_spBookMarks;
	uint64_t m_uTotalLen;
	std::shared_ptr<BookMarksTreeNode> m_spBookMarkTreeRoot;
};
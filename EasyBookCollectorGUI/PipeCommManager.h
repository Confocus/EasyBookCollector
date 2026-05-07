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
private:
	std::queue<std::string> m_qGUICommand;
	std::mutex m_mutex;
	std::shared_ptr<char[]> m_spBookMarks;
};


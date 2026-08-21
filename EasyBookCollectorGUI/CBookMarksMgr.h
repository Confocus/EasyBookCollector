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

class CBookMarksMgr
{
public:
	CBookMarksMgr();
	~CBookMarksMgr();
	VOID InsertFolder(const std::wstring);
	VOID InsertBookInfoUnderFolder(const std::wstring, const std::wstring, int64_t nFatherNum);
	std::vector<CBookMarksNode>& GetAllBookMarksNodes();
	uint64_t GetBookMarksCnt();
	std::optional<CBookMarksNode> FindIndexById(uint64_t uid);
	VOID InsertNewAddedNode();
	CBookMarksNode& GetCurrentNode();
private:
	uint64_t m_uBookMarkNodeId = 0;
	//int64_t m_uCurrentPointer;//现在遍历到哪个目录了，方便直接插入数据
	CBookMarksNode m_uCurrentNode;
	std::vector<CBookMarksNode> m_vecNodes;//每一个文件夹或文件都被当做一个Node保存到了这个数组里
	std::vector<CBookMarksNode> m_vecLastNodes;
	std::vector<std::wstring> m_vecLastFolders;//保存上一次操作的文件夹路径序列，便于判断下一次从哪开始插入
	int64_t m_nLastFatherNum;
	//std::wstring sFolderName;//文件夹的名字、自己的名字
	//std::vector<std::wstring> vecBooks;
	//std::vector<std::shared_ptr<BookMarksTree*>> vecFolders;
};

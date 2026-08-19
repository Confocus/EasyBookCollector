#pragma once
#include "framework.h"

class BookMarksNode
{
public:
	BookMarksNode() :
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
	BookMarksNode m_uCurrentNode;
	std::vector<BookMarksNode> m_vecBookMarkNodes;
	//算法细节中使用
	std::vector<BookMarksNode> m_vecLastNodes;
	//算法细节中使用：保存上一次操作的文件夹路径序列，便于判断下一次从哪开始插入
	std::vector<std::wstring> m_vecLastFolders;
	//算法细节中使用
	int64_t m_nLastFatherNum;
};
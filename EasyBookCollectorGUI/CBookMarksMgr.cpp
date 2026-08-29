#include "CBookMarksMgr.h"
CBookMarksMgr::CBookMarksMgr() :
	m_nLastFatherNum(-1)
{

}

CBookMarksMgr::~CBookMarksMgr()
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
VOID CBookMarksMgr::InsertFolder(const std::wstring s)
{
	//std::unique_lock<std::shared_mutex> lock(m_rwVecNodes);

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
	//对比前一次处理的Folders，如果有公共部分则把公共部分排除掉
	//[书签菜单]
	//[书签菜单 / 2026 / 投研]
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
		CBookMarksNode LastNode;
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
		if (i < min(vecFolders.size(), m_vecLastFolders.size()) && vecFolders[i] == m_vecLastFolders[i])
		{
			continue;
		}

		CBookMarksNode node;
		// 截取一段
		node.m_bIsFolder = true;
		node.m_nFatherNum = m_nLastFatherNum;
		node.m_uNum = m_vecThreadSafeNodes.GetSize() + 1;//计数从1开始

		m_nLastFatherNum = node.m_uNum;
		node.m_sName = vecFolders[i];
		node.m_uId = m_uBookMarkNodeId++;
		m_vecThreadSafeNodes.Append(node);
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
VOID CBookMarksMgr::InsertBookInfoUnderFolder(const std::wstring name, const std::wstring des, int64_t nFatherNum)
{
	//std::unique_lock<std::shared_mutex> lock(m_rwVecNodes);
	CBookMarksNode node;
	// 截取一段
	node.m_bIsFolder = false;
	node.m_sName = name;
	node.m_sDescription = des;
	node.m_uNum = m_vecThreadSafeNodes.GetSize() + 1;//计数从1开始
	node.m_uId = m_uBookMarkNodeId++;//这玩意儿有用吗？通过uId查找节点的时候需要一个唯一编号
	node.m_nFatherNum = nFatherNum;
	m_vecThreadSafeNodes.Append(node);
}

const std::vector<CBookMarksNode>& CBookMarksMgr::GetAllBookMarksNodes() const
{
	return m_vecThreadSafeNodes.GetData();
}

const uint64_t& CBookMarksMgr::GetBookMarksCnt() const
{
	return m_vecThreadSafeNodes.GetSize();
}

std::optional<CBookMarksNode> CBookMarksMgr::FindIndexById(uint64_t uid)
{
	return m_vecThreadSafeNodes.FindIf([uid](const CBookMarksNode& item) {
		return item.m_uId == uid;
		});
}

VOID CBookMarksMgr::InsertNewAddedNode()
{

}

CBookMarksNode& CBookMarksMgr::GetCurrentNode()
{
	return m_uCurrentNode;
}
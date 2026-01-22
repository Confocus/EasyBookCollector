#include "ListBoxWindowTree.h"

CListBoxWindowTree::CListBoxWindowTree() :
	m_spRoot(std::make_shared<CListBoxWindowNode>())
{
	m_vecTree.resize(5);
}

CListBoxWindowTree::~CListBoxWindowTree()
{

}

BOOL CListBoxWindowTree::BuildListBoxWindowTree(std::shared_ptr<CListBoxWindowNode> spNode)
{
	if (0 == m_vecTree.size() || !spNode)
	{
		return FALSE;
	}
	m_spRoot = spNode;
	m_vecTree.at(0).push_back(spNode);
	return TRUE;
}
//todo:跑一下看看逻辑哪里不通
std::optional<int> CListBoxWindowTree::GetListBoxLevelBySenderHandle(HWND hSender)
{
	for (size_t i = 0; i < m_vecTree.size(); ++i)
	{
		std::vector<std::shared_ptr<CListBoxWindowNode>> sub = m_vecTree[i];//sub = std::vector<std::shared_ptr<CListBoxWindowNode>>
		//todo:修改find_if
		if (std::find_if(sub.begin(), sub.end(), [=](std::shared_ptr<CListBoxWindowNode> it) {
			return it->GetListBoxHwnd() == hSender;
			}) != sub.end())
		{
			return static_cast<int>(i); // 找到了，返回第 i 个数组
		}
	}
	return std::nullopt; // 没找到
}

BOOL CListBoxWindowTree::InsertListBoxWindowNode(int nIndex, std::shared_ptr<CListBoxWindowNode> spNode)
{
	//todo:这里查重一下
	m_vecTree[nIndex].push_back(spNode);
	return TRUE;
}
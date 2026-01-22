#pragma once
#include <Windows.h>
#include <optional>
#include <memory>
#include <vector>
#include "ListBoxWindowNode.h"
//todo:
//ListboxWindowMgr 包含一个CListBoxWindowTree
//CListBoxWindowTree管理窗口之间的对应关系
//层级关系用std::vector<std::vector<std::shared_ptr<CListBoxWindowUnit>>>表示
//每个CListBoxWindowUnit要包含一个itemlist
//每个节点要指向上级item，上级item也要指向这个unit节点。双向指针

//#define CAT(a, b) a##b          // 核心：预处理器粘合符，LEVEL + 1 = LEVEL1
//#define MAKE_LEVEL(n) #define CAT(LEVEL, n) n  // 生成单个宏：LEVELn = n
//#define _GEN_LEVEL(n, max) \    // 递归生成宏，n=起始值，max=最大值
//MAKE_LEVEL(n); \            // 生成当前LEVELn宏
//n < max ? _GEN_LEVEL(n + 1, max) : 0  // 递归终止条件：n>=max时停止
//
//	// ============ 一行调用，直接生成 LEVEL1 ~ LEVEL20 （改20为你要的最大值即可） ============
//	_GEN_LEVEL(1, 20)
class CListBoxWindowTree
{
public:
	CListBoxWindowTree();
	~CListBoxWindowTree();
	[[deprecated]]
	BOOL BuildListBoxWindowTree(std::shared_ptr<CListBoxWindowNode> spNode);
	std::optional<int> GetListBoxLevelBySenderHandle(HWND hWnd);
	std::optional<std::shared_ptr<CListBoxWindowNode>> GetNodePointerByHandle(HWND hWnd);
	BOOL InsertListBoxWindowNode(int nIndex, std::shared_ptr<CListBoxWindowNode> spNode);
private:
	CListBoxWindowTree& operator=(const CListBoxWindowTree& other) = delete;
	CListBoxWindowTree(const CListBoxWindowTree& other) = delete;
	CListBoxWindowTree(CListBoxWindowTree&& other) = delete;
private:
	std::shared_ptr<CListBoxWindowNode> m_spRoot;
	//动态构建一个vector的vector，只不过每个vector中还存放着所在层级
	std::vector<std::vector<std::shared_ptr<CListBoxWindowNode>>> m_vecTree;
};
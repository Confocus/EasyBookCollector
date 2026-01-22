#pragma once
#include <Windows.h>
#include <optional>
#include <memory>
#include <vector>
#include "ListBoxWindowNode.h"
#include "ListBoxWindowTree.h"

#define LISTBOX_WINDOW_CLASS_NAME L"ListboxWindowClass"
#define ID_SUB_LISTBOX_START 4001  

//todo:这里应该再创建一个Windows Listbox单元，而这个Manager类只负责管理
class CListBoxWndManager
{
public:
	CListBoxWndManager();
	virtual ~CListBoxWndManager();

	/**************************************************************************
	* @brief 创建表示ListBoxWindow节点的树结构，其实这里只是创建一个树的根节点，
	* @brief 所以只需要传入构建根节点的Listbox窗口句柄和Windows窗口句柄即可
	* @param HWND hWnd 发送创建窗口的Listbox所在的Windows 
	* @param HWND hListbox 发送创建窗口的Listbox
	* @return 创建是否成功
	*************************************************************************/
	BOOL BuildListBoxWindowTree(HWND hWnd, HWND hListbox);

	/**************************************************************************
	* @brief  创建一个节点
	* @param  HINSTANCE hInst
	* @param  HWND hSender 发送创建窗口的Listbox，用来记录新的ListboxWindows从属于树结构的哪一层级
	* @param  后边四个参数分别是左上角坐标以及宽度和高度
	* @return 创建节点是否成功
	*************************************************************************/
	std::optional<std::shared_ptr<CListBoxWindowNode>> CreateListBoxWindowNode(HWND hSender, int x, int y, int width, int height);//HINSTANCE hInst, 

	/**************************************************************************
	* @brief 必须先创建一个Windows窗口然后再在里面创建ListBox。我们就注册这样的一个窗口类。
	* @param hInstance
	* @return 返回注册是否成功
	*************************************************************************/
	BOOL RegisterListBoxWindowClass(HINSTANCE hInstance);

	/**************************************************************************
	* @brief 把节点插入到树中
	* @param spNode
	* @return 返回是否插入成功
	*************************************************************************/
	BOOL InsertNodeToTree(unsigned int nIndex, std::shared_ptr<CListBoxWindowNode> spNode);

	BOOL BindParentAndSonNode(unsigned int nMapIndex, std::shared_ptr<CListBoxWindowNode> spParentNode, std::shared_ptr<CListBoxWindowNode> spSonNode);

	/**************************************************************************
	* @brief 根据SenderHandle拿到节点所在的层级。
	* @param hSender
	* @return 返回所在层级
	*************************************************************************/
	std::optional<unsigned int> GetLevelBySenderHandle(HWND hSender);

	std::optional<std::shared_ptr<CListBoxWindowNode>> GetNodePointerByHandle(HWND hWnd);

	/**************************************************************************
	* @brief 用来检查某个ListBox的某个项下的ListboxWindow是否处于展示状态
	* @param hSender
	* @return 返回所在层级
	*************************************************************************/
	BOOL IsSubListBoxShowed(HWND hSender);
private:
	CListBoxWndManager(const CListBoxWndManager& other);
	CListBoxWndManager& operator=(const CListBoxWndManager& other);

private:
	BOOL mIsListBoxWindowRegistered;
	HWND m_hWindow;
	HWND m_hListBox;
	//std::vector<Item> m_vecItem;
	//std::vector<CListBoxWindowUnit> m_vecListBoxWnd;
	//这里构建一个ListBoxWndTree，来保存各个窗口之间的层级关系
	std::shared_ptr<CListBoxWindowTree> m_spTree;
};
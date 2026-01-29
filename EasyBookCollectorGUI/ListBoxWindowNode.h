#pragma once
#include <Windows.h>
#include <optional>
#include <memory>
#include <vector>
#include <map>

class CListBoxItem;

class CListBoxWindowNode
{
public:
	CListBoxWindowNode();
	CListBoxWindowNode(HWND hWindow, HWND hListBox, BOOL bIsShowed = FALSE);
	~CListBoxWindowNode();
	VOID SetFromCursel(unsigned int);
	HWND GetListBoxHwnd();
	VOID SetIsShowed(BOOL bShowed);
	BOOL GetIsShowed();
	/*VOID SetIsSubWndShowed(BOOL bShowed);
	BOOL GetIsSubWndShowed();*/

	VOID SetParentNode(std::shared_ptr<CListBoxWindowNode> spParentNode);
	std::shared_ptr<CListBoxWindowNode> GetParentNode();
	VOID AddSonNode(unsigned int nMapIndex, std::shared_ptr<CListBoxWindowNode> spSonNode);
	std::shared_ptr<CListBoxWindowNode>& GetSonNode(unsigned int nMapIndex);
	VOID SetParentListboxIndex(unsigned int nMapIndex);
	
	HWND GetCurrentHWND();
private:
	//建立关系时需要的数据
	HWND m_hWindow;
	HWND m_hListBox;
	//一个节点只有一个Parent
	std::shared_ptr<CListBoxWindowNode> m_spParentNode;
	unsigned int m_nListboxIndex;
	//todo:一个节点有多个Son还是用map？
	std::map<unsigned int, std::shared_ptr<CListBoxWindowNode>> m_mapSonNodes;
	//todo:建立一个ListItem对应的是否show的关系？

	BOOL m_IsShowed;
	//BOOL m_IsSubWndShowed;
	std::optional<unsigned int> m_FromCursel;
	//每一个ListBoxWindow所对应的上一层级的Listbox中的Item，或节点中的信息
	std::unique_ptr<CListBoxItem> m_upItem;
	
};

//目录或书籍的每一项所对应的一个ListBoxWindow
class CListBoxItem
{
private:
	int m_nCursel;
	int m_nBookname;//或者是目录名字
	//保存一个指针，指向ListBoxWindow
};
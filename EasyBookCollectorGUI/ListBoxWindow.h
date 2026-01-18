#pragma once
#include <Windows.h>
#include <optional>
#include <memory>
#include <vector>

class CListBoxItem;

class CListBoxWindowUnit
{
public:
	CListBoxWindowUnit();
	CListBoxWindowUnit(HWND hWindow, HWND hListBox, BOOL bIsShowed = FALSE);
	~CListBoxWindowUnit();
	VOID SetFromCursel(unsigned int);
private:
	HWND m_hWindow;
	HWND m_hListBox;
	BOOL m_IsShowed;
	unsigned int m_Index;
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

//todo:
//ListboxWindowMgr 包含一个CListBoxWindowTree
//CListBoxWindowTree管理窗口之间的对应关系
//层级关系用std::vector<std::vector<std::shared_ptr<CListBoxWindowUnit>>>表示
//每个CListBoxWindowUnit要包含一个itemlist
//每个节点要指向上级item，上级item也要指向这个unit节点。双向指针

class CListBoxWindowTree
{
public:
	CListBoxWindowTree();
	~CListBoxWindowTree();
	VOID BuildListBoxWindowTree(std::shared_ptr<CListBoxWindowUnit> spUnit);
private:
	std::shared_ptr<CListBoxWindowUnit> m_spRoot;
	//动态构建一个vector的vector，只不过每个vector中还存放着所在层级
	std::vector<std::vector<std::shared_ptr<CListBoxWindowUnit>>> m_vecTree;
};
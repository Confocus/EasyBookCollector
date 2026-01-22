#pragma once
#include <Windows.h>
#include <optional>
#include <memory>
#include <vector>

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
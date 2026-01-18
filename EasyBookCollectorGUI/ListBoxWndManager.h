#pragma once
#include <Windows.h>
#include <optional>
#include <vector>
#include "ListBoxWindow.h"

#define LISTBOX_WINDOW_CLASS_NAME L"ListboxWindowClass"
#define ID_SUB_LISTBOX_START 4001  

//todo:这里应该再创建一个Windows Listbox单元，而这个Manager类只负责管理
class CListBoxWndManager
{
public:
	CListBoxWndManager();
	virtual ~CListBoxWndManager();
	VOID Initialize();
	//创建一个Windows并随之携带一个ListBox
	/**************************************************************************
	* @brief  
	* @param  
	* @param  
	* @param  
	* @param  
	* @param  
	* @return 
	*************************************************************************/
	VOID CreateListBoxWindow(HINSTANCE hInst, int x, int y, int width, int height);

	/**************************************************************************
	* @brief 必须先创建一个Windows窗口然后再在里面创建ListBox。我们就注册这样的一个窗口类。
	* @param hInstance
	* @return 返回注册是否成功
	*************************************************************************/
	BOOL RegisterListBoxWindowClass(HINSTANCE hInstance);
private:
	CListBoxWndManager(const CListBoxWndManager& other);
	CListBoxWndManager& operator=(const CListBoxWndManager& other);

private:
	BOOL mIsListBoxWindowRegistered;
	HWND m_hWindow;
	HWND m_hListBox;
	//std::vector<Item> m_vecItem;
	//std::vector<CListBoxWindowUnit> m_vecListBoxWnd;
	//todo：这里构建一个ListBoxWndTree，来保存各个窗口之间的层级关系
	std::shared_ptr<CListBoxWindowTree> m_spTree;
};
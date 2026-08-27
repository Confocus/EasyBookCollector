#include "framework.h"

#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <tchar.h>
#include <thread>
#include "EasyBookCollectorGUI.h"
#include "MainWindowActions.h"
#include "ListBoxWndManager.h"
//#include "../public/PipeMgr.h"测试不同类型的管道用的
#include <shellapi.h>
#include "ListViewMgr.h"
#include <array>
#include "PipeMessageHandler.h"

#include <commctrl.h>
#include <ole2.h>
#include <shlobj.h>

#pragma comment(lib, "ole32.lib")
//#pragma comment(lib, "commctrl.lib")


#define MAX_LOADSTRING 100
const int HOVER_TIME = 300;
BOOL g_bIsTrackRegistered = FALSE;

CMainWindowActions g_MainWndActions;
CListBoxWndManager g_ListBoxWndMgr;
//CPipeMgr::CPipeServer g_PipeMgr;测试不同类型的管道用的
HWND hChildList = NULL;
//todo:这里改成一个单实例类
//CPipeCommManager g_PipeCommMgr;

#define MOUSE_LEAVE_MONITOR 2001
#define ID_MAIN_LISTBOX 3001 // 目录按钮ID
// 全局变量:
HINSTANCE g_hInstance;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名


// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

//todo:完善并理解代码
//todo:验证切换成四屏的功能
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_ERASEBKGND:
		return TRUE; // 不擦除背景，自己在WM_PAINT里绘制，解决了拖拽Splitter时ListView边缘闪烁的问题
	case WM_CREATE: 
	{
		//这里的界面一直不显示，直到等到BookRemarks的数据解析完成
		//todo：这里要改成在解析完数据之后处理
		HANDLE hLoadedBookmarksEvent = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			EVENT_NAME_LOADED_BOOKMARKS
		);

		if (hLoadedBookmarksEvent == NULL)
		{
			//todo：如何进行错误处理？
			break;
		}
		//todo：这里等待载入的时候，如何显式“载入中……”
		if (WAIT_OBJECT_0 != WaitForSingleObject(hLoadedBookmarksEvent, INFINITE))
		{
			//todo：如何进行错误处理？
			break;
		}
		CListViewMgr::instance().InitDoubleListViewAndLoadData(hWnd);

		if (hLoadedBookmarksEvent)
		{
			CloseHandle(hLoadedBookmarksEvent);
		}
		break;
	}
	case WM_SIZING:
	{
		// 用户开始拖动边框调整大小
		CListViewMgr::instance().SetBorderDraggedStatus(TRUE);
		break;
	}
	case WM_EXITSIZEMOVE:
	{
		// 用户结束了拖动（鼠标松开或按回车）
		CListViewMgr::instance().SetBorderDraggedStatus(FALSE);
		break;
	}
	case WM_SIZE: 
	{
		CListViewMgr::instance().DragSplitterAndRefreshAllListView(hWnd);
		break;
	}

	// 处理拆分条拖动
	case WM_LBUTTONDOWN: 
	{
		CListViewMgr::instance().PressSplitter(hWnd, msg, wParam, lParam);
		break;
	}
	case WM_LBUTTONUP:
	{
		CListViewMgr::instance().ReleaseSplitter(hWnd, msg, wParam, lParam);
		break;
	}
	case WM_MOUSEMOVE: 
	{
		CListViewMgr::instance().DragSplitterAndSendMessage(hWnd, msg, wParam, lParam);
		break;
	}
	case WM_NOTIFY: // 处理ListView双击（核心：进入虚拟文件夹）
	{
		NMHDR* pNMHDR = (NMHDR*)lParam;
		if (pNMHDR->code == NM_DBLCLK) //todo：这里有一个问题，就是没有判断双击的是不是ListView Item
		{
			CListViewMgr::instance().VisitListViewFolder(hWnd, msg, wParam, lParam);
		}
		else if (pNMHDR->code == LVN_GETINFOTIP)
		{
			NMLVGETINFOTIP* pTip = (NMLVGETINFOTIP*)lParam;

			if (pNMHDR->hwndFrom == CListViewMgr::instance().GetLeftListView())
			{
				NMLVGETINFOTIP* pTip = (NMLVGETINFOTIP*)lParam;
				int index = pTip->iItem;
				LVITEM lvi = { 0 };
				lvi.mask = LVIF_PARAM;   // 只取 lParam
				lvi.iItem = index;
				ListView_GetItem(CListViewMgr::instance().GetLeftListView(), &lvi);

				// 直接强转成你的 ID 类型
				UINT uId = (UINT)lvi.lParam;
				if (ID_BACK_TO_PARENT != lvi.lParam)
				{
					//todo：检查这里的算法是否正确，看看uid是否和vector下标匹配，看看uid不基于1000计算是否可以
					std::optional<CBookMarksNode> node = CListViewMgr::instance().FindIndexById(lvi.lParam);
					if (!node.has_value())
					{
						break;
					}

					swprintf_s(
						pTip->pszText,
						pTip->cchTextMax,
						L"名称：%ws\r\n描述：%ws",
						node->m_sName.c_str(),
						node->m_sDescription.c_str());
				}
				
			}
			else if (pNMHDR->hwndFrom == CListViewMgr::instance().GetRightListView())
			{
				wsprintf(pTip->pszText,
					L"右侧 ListView：第 %d 项",
					pTip->iItem);
			}
		}
		else if (pNMHDR->code == NM_RCLICK)//处理右键点击ListView Item   pNMHDR->idFrom == IDC_LISTVIEW && 
		{
			NMLISTVIEW* pNmLv = (NMLISTVIEW*)lParam;

			// pNmLv->iItem == -1：右键点击ListView空白区域，没有点到任何item
			if (pNmLv->iItem != -1)
			{
				CListViewMgr::instance().SaveInsertedFolder(pNMHDR->hwndFrom, lParam);
				// 选中被右键点击的那一行（可选，很多UI习惯右键自动选中该行）
				ListView_SetItemState(pNMHDR->hwndFrom, pNmLv->iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

				POINT pt = pNmLv->ptAction;
				// 把控件客户坐标转为屏幕坐标，TrackPopupMenu需要屏幕坐标
				ClientToScreen(pNMHDR->hwndFrom, &pt);

				//右键某个ListViewItem弹出菜单决定添加到哪里
				HMENU hPopup = CreatePopupMenu();
				AppendMenuW(hPopup, MF_STRING, ID_POPUP_ADD, L"添加");
				AppendMenuW(hPopup, MF_STRING, ID_POPUP_DELETE, L"删除");

				TrackPopupMenu(
					hPopup,
					TPM_RIGHTBUTTON | TPM_BOTTOMALIGN,
					pt.x, pt.y,
					0,
					hWnd,   // 父窗口接收菜单命令 WM\_COMMAND
					nullptr
				);
				DestroyMenu(hPopup);

			}
		}
		break;
	}
	case WM_CONTEXTMENU:
	{
		HWND hLv = (HWND)wParam;

		// 取出屏幕坐标
		int x = ((int)(short)LOWORD(lParam));
		int y = ((int)(short)HIWORD(lParam));

		// 屏幕坐标 → ListView客户区坐标
		POINT pt = { x, y };
		::ScreenToClient(hLv, &pt);

		LVHITTESTINFO ht{};
		ht.pt = pt;
		//::ListView_SubItemHitTest(hLv, &ht);

		//bool bClickOnItem = (ht.iItem != -1); // 是否点到条目

		HMENU hPopup = CreatePopupMenu();
		AppendMenuW(hPopup, MF_STRING, ID_POPUP_ADD, L"添加");
		AppendMenuW(hPopup, MF_STRING, ID_POPUP_DELETE, L"删除");

		// 加载你的弹出菜单
		/*HMENU hMenuPopup = ::LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_POPUP_MENU1));
		HMENU hSubMenu = ::GetSubMenu(hMenuPopup, 0);*/

		::EnableMenuItem(hPopup, ID_POPUP_DELETE, MF_BYCOMMAND | MF_GRAYED);
		::EnableMenuItem(hPopup, ID_POPUP_ADD, MF_BYCOMMAND | MF_ENABLED);

		// 弹出菜单，TPM_RIGHTBUTTON：右键弹出
		::TrackPopupMenu(hPopup, TPM_RIGHTBUTTON | TPM_RETURNCMD,
			x, y, 0, hWnd, nullptr);

		::DestroyMenu(hPopup);
		
		break;
	}
	// 处理Backspace返回上一级
	case WM_KEYDOWN: 
	{
		// 原有Backspace返回上一级逻辑不变...
			// F12切换双/四面板
		if (wParam == VK_F11) {
			//todo:如何在切换时仍保留当前的访问状态
			CListViewMgr::instance().TogglePanelMode(hWnd);
			break;
		}
		break;
		//todo：晚些处理
		//if (wParam == VK_BACK) {
		//	// 判断当前激活的面板
		//	HWND hFocus = GetFocus();
		//	if (hFocus == g_hLeftListView) {
		//		// 返回上一级：找到当前父节点的父节点
		//		ItemNode* curr_parent = FindVirtualFoldNode(g_left_current_parent);
		//		g_left_current_parent = curr_parent ? curr_parent->nParentId : -1;
		//		LoadVirtualFolder(g_hLeftListView, g_left_current_parent);
		//	}
		//	else if (hFocus == g_hRightListView) {
		//		ItemNode* curr_parent = FindVirtualFoldNode(g_right_current_parent);
		//		g_right_current_parent = curr_parent ? curr_parent->nParentId : -1;
		//		LoadVirtualFolder(g_hRightListView, g_right_current_parent);
		//	}
		//}
		break;
	}

	case WM_DESTROY: 
	{
		OleUninitialize();
		CListViewMgr::instance().Destory();
		PostQuitMessage(0);
		break;
	}
	case WM_COMMAND: 
	{
		UINT cmdId = LOWORD(wParam);
		//获取当前网页的信息并添加
		//NMLISTVIEW* pNMLV = (NMLISTVIEW*)lParam;
		//int nItem = pNMLV->iItem;
		//if (nItem == -1) break; // 无效

		//// ==========1. 获取Item第一列文本==========
		//TCHAR szText[256] = { 0 };
		//LVITEM lvi = { 0 };
		//lvi.iItem = nItem;
		//lvi.iSubItem = 0;
		//lvi.pszText = szText;
		//lvi.cchTextMax = _countof(szText);
		//ListView_GetItem(hWnd, &lvi);

		//// ==========2. 获取你预先绑定的自定义数据==========
		//LVITEM lvData = { 0 };
		//lvData.iItem = nItem;
		//lvData.mask = LVIF_PARAM;
		//ListView_GetItem(hWnd, &lvData);
		//LPARAM userData = lvData.lParam;
		switch (cmdId)
		{
			case ID_POPUP_DELETE:
			{
				break;
			}

			case ID_POPUP_ADD:
			{
				//如果只是连续多次右键但是并没有点击“添加”怎么办
				//但是另一个线程的处理顺序不一定是我这里，因为可能出现一种情况：
				//就是另一个处理命令的线程可能刚刚要处理，但是m_NodeTobeAdded又被右键修改了
				//实际上这个队列能保证它所保存的时序和你操作的时序是一致的 //todo：或者后面加个时间戳以进一步保证
				CPipeMessageHandler::instance().PushGUICommandToQueue(STRING_ADD_ACTIVE_TAB);
				break;
			}
		}
		break;
	}
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
	return 0;
}

// ===================== 窗口类注册 & 程序入口 =====================
ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = L"VirtualFolderDemo";
	return RegisterClassExW(&wcex);
}

//todo:考虑其它健壮性相关的问题，比如一端如果崩溃了怎么办
//todo:要考察GUI、Daemon、Firefox三个端直接不同的出错情况下或不同启动顺序下是否能够挽救回来
//启动管道通信组建
unsigned __stdcall StartCommManager(void* param)
{
	CPipeMessageHandler::instance().Run();
	return 0;
}

//todo：如果VS以管理员权限启动，好像通信会有问题
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) 
{
	InitCommonControls();
	OleInitialize(NULL);

	INITCOMMONCONTROLSEX icex = {};
	icex.dwSize = sizeof(icex);
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	if (!MyRegisterClass(hInstance)) return FALSE;

	//这个线程的位置应该往前移，移到创建窗口前面，因为创建窗口事件要等待这里拿到数据
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, StartCommManager, (void*)NULL, 0, 0);

	HWND hWnd = CreateWindowW(L"VirtualFolderDemo", L"书籍目录保存",
		WS_OVERLAPPEDWINDOW , CW_USEDEFAULT, CW_USEDEFAULT, CListViewMgr::instance().GetInitMainWndWidth(), 600,//| WS_CLIPCHILDREN
		NULL, NULL, hInstance, NULL);
	if (!hWnd) return FALSE;

	//DragAcceptFiles(hWnd, TRUE);//todo：这个可能没必要

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	CloseHandle(hThread);
	return (int)msg.wParam;
}

// 辅助函数：获取客户区宽/高
int GetClientRectWidth(HWND hWnd) {
	RECT rc; GetClientRect(hWnd, &rc); return rc.right - rc.left;
}
int GetClientRectHeight(HWND hWnd) {
	RECT rc; GetClientRect(hWnd, &rc); return rc.bottom - rc.top;
}



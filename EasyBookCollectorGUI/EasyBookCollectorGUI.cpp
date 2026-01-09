// EasyBookCollectorGUI.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EasyBookCollectorGUI.h"
#include "CMainWindowActions.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100
const int HOVER_TIME = 300;
CMainWindowActions g_MainWndActions;
BOOL g_bIsTrackRegistered = FALSE;
#define MOUSE_LEAVE_MONITOR 2001
#define IDC_BTN_DIRECTORY 3001 // 目录按钮ID

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 在此处放置代码。

	// 初始化全局字符串
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_EASYBOOKCOLLECTORGUI, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EASYBOOKCOLLECTORGUI));

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EASYBOOKCOLLECTORGUI));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;  MAKEINTRESOURCEW(IDC_EASYBOOKCOLLECTORGUI);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	HWND hWnd = CreateWindowW(szWindowClass, szTitle,
		WS_POPUP,//WS_OVERLAPPEDWINDOW
		0, 0,
		CW_USEDEFAULT, //[in]           int       nWidth,
		0,
		nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hBrush = CreateSolidBrush(RGB(230, 230, 230));
	static HWND hListBox = NULL;

	switch (message)
	{
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;

		RECT rc;
		GetClientRect(hWnd, &rc);
		FillRect(hdc, &rc, hBrush);

		return 1; // 告诉系统：我已经擦过背景了
	}
	case WM_CREATE:
	{
		//hListBox = CreateWindow(
		//	WC_LISTBOX,          // ★固定：ListBox控件类名
		//	TEXT(""),            // 控件标题(没用，ListBox内容靠AddString添加)
		//	WS_CHILD | WS_VISIBLE | LBS_NOTIFY , // ★必选样式：子控件+可见| LBS_MULTICOLUMN
		//	20, 20,              // 控件左上角坐标 X,Y (相对于父窗口客户区)
		//	200, 300,            // 控件宽高 W,H
		//	hWnd,                // 父窗口句柄
		//	(HMENU)3001,         // ★控件ID(自定义，比如1001，唯一即可，用于响应事件)
		//	(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), // 程序实例句柄
		//	NULL                 // 扩展参数，固定为NULL
		//);
		hListBox = CreateWindowEx(
			0,  // ★扩展样式：0 = 无边框，WS_EX_CLIENTEDGE = 有边框
			WC_LISTBOX,
			TEXT(""),
			WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
			20, 20, 200, 300,
			hWnd, (HMENU)1001, hInst, NULL
		);
		SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"第一项");
		SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"第二项");
		SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)L"第三项");
		//SendMessage(hListBox, LB_SETCOLUMNWIDTH, 80, 0);

		//INITCOMMONCONTROLSEX icc = {};
		//icc.dwSize = sizeof(icc);
		//icc.dwICC = ICC_TREEVIEW_CLASSES;
		//InitCommonControlsEx(&icc);

		//HWND hTree = CreateWindowW(
		//	WC_TREEVIEW, _T("目录"),
		//	WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
		//	100, 100, 100, 100,
		//	hWnd, (HMENU)3001,
		//	(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		//	NULL);
		////插入一个根节点
		//TVINSERTSTRUCTW tvis = {};
		//tvis.hParent = TVI_ROOT;
		//tvis.hInsertAfter = TVI_LAST;
		//tvis.item.mask = TVIF_TEXT;
		//tvis.item.pszText = (LPWSTR)L"Root";

		//HTREEITEM hRoot = (HTREEITEM)SendMessageW(
		//	hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis
		//);
		//// 插入一个子节点
		//tvis.hParent = hRoot;
		//tvis.item.pszText = (LPWSTR)L"Child";

		//SendMessageW(hTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);

		//HWND hTree2 = CreateWindowW(
		//	WC_TREEVIEW, _T("目录2"),
		//	WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS,
		//	200, 100, 100, 100,
		//	hWnd, (HMENU)3002,
		//	(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		//	NULL);
		////插入一个根节点
		//TVINSERTSTRUCTW tvis2 = {};
		//tvis2.hParent = TVI_ROOT;
		//tvis2.hInsertAfter = TVI_LAST;
		//tvis2.item.mask = TVIF_TEXT;
		//tvis2.item.pszText = (LPWSTR)L"Root2";
		//HTREEITEM hRoot2 = (HTREEITEM)SendMessageW(
		//	hTree2, TVM_INSERTITEMW, 0, (LPARAM)&tvis2
		//);

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);   // 屏幕宽度
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);  // 屏幕高度
		int width = screenWidth / 5;   // 新宽度
		int height = screenHeight / 2;  // 新高度

		SetWindowPos(hWnd, NULL, screenWidth - width, 100, 
			width, height,
			 SWP_NOZORDER);//SWP_NOMOVE

		g_nEdgeWidth = width / 20;
		SetTimer(hWnd, MOUSE_LEAVE_MONITOR, 20, NULL);
		break;
	}
	case WM_TIMER:
	{
		UINT uTimerID = (UINT)wParam; // 获取触发的定时器ID
		switch (uTimerID)
		{
		case ANIMATE_TIMER_ID:
		{
			g_MainWndActions.ProcessStimulateSlideHideWindowToRightEdge(hWnd);
			break;
		}
		case MOUSE_LEAVE_MONITOR:
		{
			
			//有一种情况鼠标移动出窗口不会隐藏，那就是：
		// 鼠标沿着主窗口的右边框往上或往下移动
		if (g_MainWndActions.GetObCursorOnRightEdge())
		{
			POINT ptMouse;
			GetCursorPos(&ptMouse);
			std::optional<int> nTop = g_MainWndActions.GetMainWindowTop(hWnd).value();
			std::optional<int> nBottom = g_MainWndActions.GetMainWindowBottom(hWnd).value();
			if ((nTop.has_value() && nTop.value() > ptMouse.y ) || (nBottom.has_value() && nBottom.value() < ptMouse.y))
			{
				//触发移动
				g_MainWndActions.SetObCursorOnRightEdge(FALSE);
				g_MainWndActions.StartStimulateSlideHideWindowToRightEdge(hWnd);
			}
		}
			break;
		}
		default:
			break;
		}

		break;
	}
	case WM_WINDOWPOSCHANGING:
	{
		break;
	}
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		if (LOWORD(wParam) == 3001)
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				// 选中项改变
					int index = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
					if (index != LB_ERR)
					{
						wchar_t text[256];
						SendMessage(hListBox, LB_GETTEXT, index, (LPARAM)text);
						MessageBox(hWnd, text, L"你选中了", MB_OK);
					}
			}
			else if (HIWORD(wParam) == LBN_DBLCLK)
			{
				// 双击
			}
		}
		// 分析菜单选择:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		if (LOWORD(wParam) == IDC_BTN_DIRECTORY && HIWORD(wParam) == BN_CLICKED)
		{

		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_SIZE: 
	{  // 窗口大小改变时
		
		break;
	}
	case WM_MOUSEMOVE:
	{
		//这段代码是向 Windows 系统注册 “鼠标离开窗口” 和 “鼠标悬停在窗口内” 的监听，让窗口能收到 WM_MOUSELEAVE 和 WM_MOUSEHOVER 这两个原本不会主动触发的消息。
		if (!g_bIsTrackRegistered)
		{
			TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
			tme.dwFlags = TME_LEAVE | TME_HOVER; // 同时监听悬停和离开 | 
			tme.hwndTrack = hWnd;
			tme.dwHoverTime = HOVER_TIME; // 300ms悬停触发
			TrackMouseEvent(&tme);
			g_bIsTrackRegistered = TRUE;
		}

		
		break;
	}
	case WM_MOUSEHOVER:
	{
		g_bIsTrackRegistered = FALSE;
		if (g_bIsMainWindowHide == TRUE)
		{
			g_MainWndActions.ShowHidedWindowFromRightSide(hWnd);
		}
		break;
	}
	case WM_MOUSELEAVE:
	{
		do 
		{
			g_bIsTrackRegistered = FALSE;
			std::optional<BOOL> bTouchRightEdge = g_MainWndActions.IsMainWindowTouchScreenEdge(hWnd);
			std::optional<BOOL> bAtRight = g_MainWndActions.IsMouseOnMainWindowRightEdge(hWnd);
			//保证窗口贴着边，且鼠标不在窗口右侧才隐藏 
			//std::optional 的取反针对「是否有值」，而非「值的内容」
			//BOOL b = bAtRight.value();
			if (bAtRight.value())
			{
				g_MainWndActions.SetObCursorOnRightEdge(TRUE);
				break;
			}

			//如果是悬浮在某个子控件上
			if (!g_MainWndActions.IsMouseReallyLeaveMainWnd(hWnd))
			{
				break;
			}

			//如果触碰了右边边界，且鼠标不在在右边边界附近即是从其它方向挪出来的
			//有一种情况会出现Bug，就是鼠标先快速从上面移出，在窗口移动的过程中，再迅速让鼠标从左边移出，
			//这样记录的就是正在移动的过程中的窗口的左上角坐标，此时再赋值给g_nOriginalWindowLeft就不是
			//窗口默认打开时候的位置了
			if (bTouchRightEdge && !bAtRight.value())//  
			{
				//隐藏窗口
				g_MainWndActions.StartStimulateSlideHideWindowToRightEdge(hWnd);
				g_MainWndActions.SetObCursorOnRightEdge(FALSE);
				break;
			}
			
		} while (0);
		
		break;
	}
	case WM_DESTROY:
		KillTimer(hWnd, MOUSE_LEAVE_MONITOR);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
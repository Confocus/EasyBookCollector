// EasyBookCollectorGUI.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <tchar.h>
#include "EasyBookCollectorGUI.h"
#include "MainWindowActions.h"
#include "ListBoxWndManager.h"

#define MAX_LOADSTRING 100
const int HOVER_TIME = 300;
BOOL g_bIsTrackRegistered = FALSE;


CMainWindowActions g_MainWndActions;
CListBoxWndManager g_ListBoxWndMgr;
HWND hChildList = NULL;

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

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	OutputDebugStringW(L"[bookcollector] wWinMain start++\n");

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// 初始化全局字符串
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_EASYBOOKCOLLECTORGUI, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	g_ListBoxWndMgr.RegisterListBoxWindowClass(hInstance);
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
	g_hInstance = hInstance; // 将实例句柄存储在全局变量中

	//得通过 WM_LBUTTONDOWN 发送 WM_SYSCOMMAND 消息，模拟标题栏拖动
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

//std::optional<int> GetListBoxLevel(const std::vector<std::vector<HWND>>& vecListboxHwnd, HWND hWnd) {
//	for (size_t i = 0; i < vecListboxHwnd.size(); ++i) {
//		const auto& sub = vecListboxHwnd[i];
//		if (std::find(sub.begin(), sub.end(), hWnd) != sub.end()) {
//			return static_cast<int>(i); // 找到了，返回第 i 个数组
//		}
//	}
//	return std::nullopt; // 没找到
//}

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
	static HWND hMainListBox = NULL;

	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		// 发送 WM_SYSCOMMAND 消息，模拟标题栏拖动
		// SC_MOVE：移动窗口命令；HTCAPTION：强制按标题栏区域处理
		SendMessageW(hWnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
		return 0;
	}
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
		WNDCLASSEX wcCheck = { 0 };
		wcCheck.cbSize = sizeof(WNDCLASSEX);
		if (GetClassInfoEx(g_hInstance, LISTBOX_WINDOW_CLASS_NAME, &wcCheck))
		{
			OutputDebugStringW(L"[bookcollector]自定义ListBox类注册成功！WndProc地址正确！\n");
		}
		else
		{
			OutputDebugStringW(L"[bookcollector]自定义类注册后不存在！\n");
		}

		hMainListBox = CreateWindowEx(
			0,  // ★扩展样式：0 = 无边框，WS_EX_CLIENTEDGE = 有边框
			WC_LISTBOX,
			TEXT(""),
			WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,//  LBS_OWNERDRAWFIXED// 自定义绘制的最优样式组合（必记，你的需求专属）ListBox 的“绘制责任”已经完全交给你了，
			//如果你不处理 WM_DRAWITEM，系统就什么都不会画。
			20, 20, 200, 300,
			hWnd, (HMENU)ID_MAIN_LISTBOX, g_hInstance, NULL
		);
		
		//set the height of item
		SendMessage(hMainListBox, LB_SETITEMHEIGHT, 0, 40);
		
		int idx = 0; 
		//所以这里加个static即可
		static std::vector<std::wstring> vItem = { L"临时存放", L"优先", L"核心能力", L"核心能力但不那么好", L"非核心能力" , L"其它" };//
		std::for_each(vItem.begin(), vItem.end(), [&](const std::wstring& item) {
			unsigned int itemIndex = static_cast<unsigned int>(SendMessage(hMainListBox, LB_ADDSTRING, 0, (LPARAM)item.c_str()));
			SendMessage(hMainListBox, LB_SETITEMDATA, itemIndex, (LPARAM)LISTBOX_INDEX_START + idx++);
			});

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);   // 屏幕宽度
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);  // 屏幕高度
		int width = screenWidth / 5;   // 新宽度
		int height = screenHeight / 2;  // 新高度

		SetWindowPos(hWnd, NULL, screenWidth - width, 100, 
			width, height,
			 SWP_NOZORDER);//SWP_NOMOVE

		g_nEdgeWidth = width / 20;
		SetTimer(hWnd, MOUSE_LEAVE_MONITOR, 20, NULL);

		//创建完成时，初始化，构建根节点，记录各个ListBox和Window的关系
		//只在根节点创建的时候Build即可
		std::optional<std::shared_ptr<CListBoxWindowNode>> spRoot = g_ListBoxWndMgr.BuildListBoxWindowTree(hWnd, hMainListBox);
		if (!spRoot.has_value() || !*spRoot) //一个是判断有没有值一个是判断值是否为NULL
		{
			//todo:增加错误日志模块
			//std::cerr << "错误：构建 CListBoxWindowTree 失败，程序将退出！" << std::endl;
			PostQuitMessage(0); // 参数是退出码，会被传递给 GetMessage 的返回值，通常传 0 即可
			return 0;
		}

		break;
	}
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
		// 只处理我们的ListBox控件
		//if (pDIS->CtlID != ID_LISTBOX || pDIS->itemID == LB_ERR) break;
		//std::optional<int> nIndex = GetListBoxLevel(g_vListBoxHwnd, pDIS->hwndItem);

		/*if (!nIndex.has_value())
		{
			break;
		}*/

		/*if (hChildList == pDIS->hwndItem)
		{
			OutputDebugStringW(L"[bookcollector]test！\n");
		}*/

		//todo:确认下这里应该不用判断吧？这里是WndProc应该就是主窗口才进来
		//if (0 == nIndex)//主ListBoxpDIS->hwndItem == g_hMainListBox
		{
			HDC hdc = pDIS->hDC;
			RECT rc = pDIS->rcItem;  // 获取当前项的原始绘制矩形
			int nItem = pDIS->itemID;// 当前项的索引

			// ✅ ✅ ✅ 核心代码【实现项间距】：向内收缩矩形，留出空白
			rc.top += 3;     // 上边距：3像素
			rc.bottom -= 3;  // 下边距：3像素
			rc.left += 6;    // 左边距：6像素
			rc.right -= 6;   // 右边距：6像素

			// ✅ 绘制项的背景：选中时淡蓝色高亮，未选中时纯白色
			HBRUSH hBrush;
			if (pDIS->itemState & ODS_SELECTED)
			{
				hBrush = CreateSolidBrush(RGB(202, 220, 250)); // Win11淡蓝色高亮，不刺眼
			}
			else
			{
				hBrush = CreateSolidBrush(RGB(255, 255, 255)); // 纯白色背景
			}
			FillRect(hdc, &rc, hBrush);
			DeleteObject(hBrush); // 释放画笔，防止内存泄漏

			// ✅ 绘制文件夹小图标
			//DrawIcon(hdc, rc.left + 5, rc.top + 3, hFolderIcon);

			// ✅ 绘制项的文字：避开图标，左对齐，黑色文字，透明背景
			int nLen = static_cast<int>(SendMessage(pDIS->hwndItem, LB_GETTEXTLEN, nItem, 0));
			std::wstring wsText;
			wsText.resize(nLen + 1);
			SendMessage(pDIS->hwndItem, LB_GETTEXT, nItem, (LPARAM)wsText.data());//传递&p更改的是p的值，那么传szBuff,更改的不就是*szBuff的值了么
			//wBuff.resize(wcslen(wBuff.data())); // 去掉多余 '\0'
			SetBkMode(hdc, TRANSPARENT);          // 文字背景透明，必加
			SetTextColor(hdc, RGB(20, 20, 20));   // 深灰色文字，比纯黑更柔和
			// 文字绘制区域：向右偏移35像素，避开图标
			RECT rcText = rc;
			rcText.left += 35;
			DrawText(hdc, wsText.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);


			return TRUE; // 告诉系统：自己绘制完成，无需默认绘制
		}
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
			//如果主窗口已经隐藏了，就不必进来了，否则浪费时间
			if (g_bIsMainWindowHide)
			{
				break;
			}
			//如果已经通知了要去ProcessStimulateSlideHideWindowToRightEdge就不该再进来处理了
			if (g_MainWndActions.GetObNotifiedStimulate())
			{
				break;
			}
			std::optional<BOOL> bRet = g_MainWndActions.IsMainWindowTouchScreenEdge(hWnd);
			//todo:如果没挨着右边则不必考虑，但是这样这里总得计算
			//todo:所以这里应该优化掉，只有当窗口贴到有彼岸时，这里才接到通知进行计时；窗口剥离右边的时候，不执行这里直接跳出
			if (bRet.has_value() && FALSE == bRet.value())
			{
				break;
			}
			//之前有一个Bug：有一种情况鼠标移动出窗口不会隐藏，那就是：鼠标沿着主窗口的右边框往上或往下移动
			//刚刚在之前被判定为曾在窗口右边缘待过，只有曾在右边缘待过才会考虑这种情况，所以大多数情况不会进入这个if
			//情况一：待完后没触发WM_MOUSELEAVE消息中的判定从左上下移出（可能还在主窗口上），那这里自然不必执行
			//情况二：待完后沿着右边往上或往下移动出了窗口
			//我不希望这里进入太频繁，所以尽量if筛选跳出
			//if (g_MainWndActions.GetObCursorOnRightEdge())
			{
				POINT ptMouse;
				GetCursorPos(&ptMouse);
				/*std::optional<int> nTop = g_MainWndActions.GetMainWindowTop(hWnd).value();
				std::optional<int> nBottom = g_MainWndActions.GetMainWindowBottom(hWnd).value();*/
				RECT rcWindow;
				if (!GetWindowRect(hWnd, &rcWindow))
				{
					break;
				}

				/*if ((nTop.has_value() && nTop.value() > ptMouse.y) 
					|| (nBottom.has_value() && nBottom.value() < ptMouse.y))*/
				if(rcWindow.top > ptMouse.y || rcWindow.bottom < ptMouse.y || rcWindow.left > ptMouse.x	)
				{
					//触发移动
					g_MainWndActions.SetObCursorOnRightEdge(FALSE);
					g_MainWndActions.NotifyStimulateSlideHideWindowToRightEdge(hWnd);
				}
			}
			break;
		}
		default:
			break;
		}

		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == LBN_SELCHANGE)//点击主窗口的Listbox选项会走到这里
		{
			//todo:这里要判断如果已经有子窗口显示了，在显示第二个子窗口的时候，要先关闭第一个
			g_ListBoxWndMgr.ShowOrHideNode(hWnd, message, wParam, lParam);
			break;
		}
		else if (HIWORD(wParam) == LBN_DBLCLK)
		{
			// 双击
		}

		// 分析菜单选择:
		switch (LOWORD(wParam))
		{
		case IDM_ABOUT:
			DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_SIZE: 
	{  // 窗口大小改变时
		
		break;
	}
	case WM_MOUSEMOVE:
	{
		//这段代码是向 Windows 系统注册 “鼠标离开窗口” 和 “鼠标悬停在窗口内” 的监听，
		//让窗口能收到 WM_MOUSELEAVE 和 WM_MOUSEHOVER 这两个原本不会主动触发的消息。
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
		//todo:我想把这段代码去掉，否则在Timer中也有类似的逻辑，以后修改得修改Timer和WM_MOUSELEAVE两处
		do 
		{
			g_bIsTrackRegistered = FALSE;//这里保留以便每次WM_MOUSELEAVE会消耗一个TRACKMOUSEEVENT后能再次初始化一个新的TRACKMOUSEEVENT
			/*std::optional<BOOL> bMainWndTouchRightEdge = g_MainWndActions.IsMainWindowTouchScreenEdge(hWnd);
			std::optional<BOOL> bCursorAtRightEdge = g_MainWndActions.IsMouseOnMainWindowRightEdge(hWnd);*/
			//保证窗口贴着边，且鼠标不在窗口右侧才隐藏 
			//鼠标此时贴着窗口右边，不执行隐藏
			/*if (bCursorAtRightEdge.value())
			{
				g_MainWndActions.SetObCursorOnRightEdge(TRUE);
				break;
			}*/

			//如果是悬浮在某个子控件上不隐藏
			/*if (!g_MainWndActions.IsMouseReallyLeaveMainWnd(hWnd))
			{
				break;
			}*/

			//todo:使用了Timer监控鼠标位置后这里的逻辑好像就不太需要了？
			//如果触碰了右边边界，且鼠标不在在右边边界附近即是从其它方向挪出来的
			//有一种情况会出现Bug，就是鼠标先快速从上面移出，在窗口移动的过程中，再迅速让鼠标从左边移出，
			//这样记录的就是正在移动的过程中的窗口的左上角坐标，此时再赋值给g_nOriginalWindowLeft就不是
			//窗口默认打开时候的位置了
			//if (bMainWndTouchRightEdge && !bCursorAtRightEdge.value())//考虑鼠标是从左上下方向移出的窗口，直接通知隐藏  
			//{
			//	//隐藏窗口
			//	g_MainWndActions.NotifyStimulateSlideHideWindowToRightEdge(hWnd);
			//	g_MainWndActions.SetObCursorOnRightEdge(FALSE);//从左上下移出后肯定鼠标就不再在主窗口边缘了
			//	break;
			//}
			
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
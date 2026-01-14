// EasyBookCollectorGUI.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EasyBookCollectorGUI.h"
#include "CMainWindowActions.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <tchar.h>

#define MAX_LOADSTRING 100
const int HOVER_TIME = 300;
CMainWindowActions g_MainWndActions;
BOOL g_bIsTrackRegistered = FALSE;
const int g_nListSpace = 10;
const int g_nListWidth = 180;
const int g_nListHeight = 300;
std::vector<std::vector<HWND>> g_vListBoxHwnd;
#define MY_LISTBOX_CLASS_NAME L"MyOwnerDrawListBox_2026"
HWND hChildList = NULL;

#define MOUSE_LEAVE_MONITOR 2001
#define ID_LISTBOX 3001 // 目录按钮ID

WNDPROC g_pOldListBoxProc = NULL;
LRESULT CALLBACK SubListBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL RegisterMyListBoxClass(HINSTANCE hInst)
{
	// 1. 获取【系统默认WC_LISTBOX的窗口类信息】：核心！继承系统ListBox的所有特性
	WNDCLASSEX wcSys = { 0 };
	wcSys.cbSize = sizeof(WNDCLASSEX);
	// 从系统获取WC_LISTBOX的完整配置，我们只需要修改窗口过程，其他全部复用
	if (!GetClassInfoEx(NULL, WC_LISTBOX, &wcSys))
	{
		return FALSE;
	}

	// 2. 基于系统配置，创建我们的自定义窗口类配置，只改3个关键属性
	WNDCLASSEX wcMy = wcSys;
	wcMy.cbSize = sizeof(WNDCLASSEX);
	wcMy.hInstance = hInst;                // 绑定你的程序实例句柄
	wcMy.lpszClassName = MY_LISTBOX_CLASS_NAME;// ✅ 改成你的自定义类名
	wcMy.lpfnWndProc = SubListBoxProc;  // ✅ 核心！绑定你的窗口过程
	wcMy.style |= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcMy.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcMy.cbWndExtra = 4;
	// 3. 向系统注册这个自定义类，注册成功返回TRUE，失败返回FALSE
	ATOM atom = RegisterClassEx(&wcMy);
	if (atom == 0)
	{
		return FALSE;
	}

	// ✅ 关键：保存系统默认的ListBox窗口过程地址（用于后续CallWindowProc）
	g_pOldListBoxProc = wcSys.lpfnWndProc;

	return TRUE;
}

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
ATOM RegisterWindow2Class(HINSTANCE hInstance);
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
	RegisterMyListBoxClass(hInstance);
	RegisterWindow2Class(hInstance);
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
LRESULT CALLBACK WndProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

ATOM RegisterWindow2Class(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc2;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EASYBOOKCOLLECTORGUI));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"Window2Class"; 
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

std::optional<int> FindListBoxLevel(const std::vector<std::vector<HWND>>& vec2d, HWND hWnd) {
	for (size_t i = 0; i < vec2d.size(); ++i) {
		const auto& sub = vec2d[i];
		if (std::find(sub.begin(), sub.end(), hWnd) != sub.end()) {
			return static_cast<int>(i); // 找到了，返回第 i 个数组
		}
	}
	return std::nullopt; // 没找到
}


LRESULT CALLBACK WndProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		// ✅ 核心：处理ListBox2的WM_DRAWITEM自绘消息，【直接复制你主窗口的代码，一行不改】
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
		if (pDIS->CtlID == 3003 && pDIS->CtlType == ODT_LISTBOX && pDIS->itemID != LB_ERR)
		{
			HDC hdc = pDIS->hDC;
			RECT rc = pDIS->rcItem;
			int nItem = pDIS->itemID;

			// ✅ 你的原有绘制逻辑：项间距，完全复用
			rc.top += 3;
			rc.bottom -= 3;
			rc.left += 6;
			rc.right -= 6;

			// ✅ 你的原有绘制逻辑：背景色+高亮，完全复用
			HBRUSH hBrush;
			if (pDIS->itemState & ODS_SELECTED)
			{
				hBrush = CreateSolidBrush(RGB(202, 220, 250)); // Win11淡蓝色高亮
			}
			else
			{
				hBrush = CreateSolidBrush(RGB(0, 255, 255)); // 纯白色背景
			}
			FillRect(hdc, &rc, hBrush);
			DeleteObject(hBrush);

			// ✅ 你的原有绘制逻辑：文字获取+绘制，完全复用（已修复为数组接收，安全无崩溃）
			WCHAR szBuff[256] = { 0 };
			SendMessage(pDIS->hwndItem, LB_GETTEXT, nItem, (LPARAM)szBuff);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(20, 20, 20));
			RECT rcText = rc;
			//rcText.left += 35;
			DrawText(hdc, szBuff, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			return TRUE; // 告诉系统绘制完成
		}
		break;
	}
	// ✅ 处理ListBox2的选中事件 LBN_SELCHANGE，和你之前的逻辑一致
	case WM_COMMAND:
	{
		//if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == ID_LISTBOX2)
		//{
		//	int nSel = SendMessage(g_hListBox2, LB_GETCURSEL, 0, 0);
		//	if (nSel != LB_ERR)
		//	{
		//		ShowWindow(hWnd, SW_HIDE); // 选中后隐藏窗口2，体验最佳
		//	}
		//}
		break;
	}
	// ✅ 窗口2关闭时，销毁句柄，防止泄漏
	case WM_DESTROY:
	{
		/*g_hWnd2 = NULL;
		g_hListBox2 = NULL;*/
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK SubListBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		OutputDebugStringW(L"[bookcollector]成功进入自定义MyGlobalListBoxProc！\n");
		SendMessage(hWnd, LB_SETITEMHEIGHT, 0, 28);
//		g_vListBoxHwnd.at(1).push_back(hWnd);
		//if (hChildList)
		{
			//g_vListBoxHwnd.at(1).push_back(hWnd);
			////// 5. 准备填充下一级数据
			//std::vector<const TCHAR*> vecChildData = { TEXT("text"), TEXT("text"), TEXT("text"), TEXT("text") };
			//for (size_t i = 0; i < vecChildData.size(); i++)
			//	SendMessage(hWnd, LB_ADDSTRING, 0, (LPARAM)vecChildData[i]);

			// 4. 子类化绑定通用Proc（关键：让ListBox能自绘）
			//if (g_pOldListBoxProc == NULL)
			//{
			//	//把系统默认给 ListBox 的「消息处理函数」，替换成你自己写的 MyGlobalListBoxProc；
			//	//同时把「被换掉的系统原函数地址」保存到 g_pOldListBoxProc 变量里，方便后续调用。
			//	g_pOldListBoxProc = (WNDPROC)SetWindowLongPtr(hChildList, GWLP_WNDPROC, (LONG_PTR)SubListBoxProc);
			//}
			//else
			//{
			//	SetWindowLongPtr(hChildList, GWLP_WNDPROC, (LONG_PTR)SubListBoxProc);
			//}

			// 5. 强制刷新，避免白屏
			//UpdateWindow(hWnd);
		}
		break;
	}
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
		if (pDIS->CtlType == ODT_LISTBOX)
		{
			OutputDebugStringW(L"[bookcollector]终于收到WM_DRAWITEM！\n");
			LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
			// 只处理我们的ListBox控件
			//if (pDIS->CtlID != ID_LISTBOX || pDIS->itemID == LB_ERR) break;
			std::optional<int> nIndex = FindListBoxLevel(g_vListBoxHwnd, pDIS->hwndItem);
			if (!nIndex.has_value())
			{
				break;
			}
			// 你的自绘逻辑（之前写的代码直接复制过来）
			if (1 == nIndex)
			{
				HDC hdc = pDIS->hDC;
				RECT rc = pDIS->rcItem;  // 获取当前项的原始绘制矩形
				int nItem = pDIS->itemID;// 当前项的索引

				rc.top += 3;     // 上边距：3像素
				rc.bottom -= 3;  // 下边距：3像素
				rc.left += 6;    // 左边距：6像素
				rc.right -= 6;   // 右边距：6像素

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

				int len = SendMessage(pDIS->hwndItem, LB_GETTEXTLEN, nItem, 0);
				std::wstring wBuff(256, L'\0');

				wchar_t* p = NULL;//传递一个指针，然后拿到指向那些字符串常量的地址
				SendMessage(pDIS->hwndItem, LB_GETTEXT, nItem, (LPARAM)&p);//传递&p更改的是p的值，那么传szBuff,更改的不就是*szBuff的值了么
				SetBkMode(hdc, TRANSPARENT);          // 文字背景透明，必加
				SetTextColor(hdc, RGB(20, 20, 20));   // 深灰色文字，比纯黑更柔和
				RECT rcText = rc;
				rcText.left += 35;
				DrawText(hdc, p, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			}
			return TRUE;
		}
		break;
	}
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			// 选中项改变
			MessageBox(NULL, L"", L"", 0);
		}
	else if (HIWORD(wParam) == LBN_DBLCLK)
	{
		// 双击
	}
		// 你的选中通知逻辑（之前写的代码直接复制过来）
		MessageBox(NULL, L"", L"", 0);
		break;
	}
	case WM_MOUSELEAVE:
	{
		MessageBox(NULL, L"", L"", 0);
		break;
	}
	case WM_DESTROY:
		// 解绑子类化，防止泄漏
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)g_pOldListBoxProc);
		break;
	}
	// 其他消息交给系统默认Proc处理
	return CallWindowProc(g_pOldListBoxProc, hWnd, uMsg, wParam, lParam);
}

VOID CreateSubListBox(HWND hSenderList, HWND hWnd)
{
	RECT rcListBox;
	GetWindowRect(hSenderList, &rcListBox);
	//TODO:子ListBox没有随主窗口的滑动而滑动
	// 
	//// 4. 动态创建【下一级】ListBox（ID自动分配，无需关心具体值）
	//TODO:再确认只有CHILD窗口才行；再采用GPT提供的方案
	hChildList = CreateWindowEx(
		0, // 扩展风格
		WC_LISTBOX,//MY_LISTBOX_CLASS_NAME
		L"这是一个弹出窗口", 
		WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_BORDER | LBS_NOINTEGRALHEIGHT  | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS, // WS_POPUP
		-20,-20,40,40,//100, 100, 300, 150,rcListBox.left - 3 * g_nListSpace - g_nListWidth, rcListBox.top, g_nListWidth, g_nListHeight,
		hWnd,
		NULL, 
		hInst,
		NULL 
	);

	if (hChildList)
	{
		/*WCHAR szClassName[256] = { 0 };
		GetClassName(hChildList, szClassName, 256);
		if (wcscmp(szClassName, MY_LISTBOX_CLASS_NAME) == 0)
		{
			OutputDebugStringW(L"[bookcollector]子ListBox成功使用自定义类！\n");
		}
		else
		{
			OutputDebugStringW(L"[bookcollector]子ListBox用的是系统类！实际类名：");
			OutputDebugStringW(szClassName);
			OutputDebugStringW(L"\n");
		}*/

		g_vListBoxHwnd.at(1).push_back(hChildList);
		SendMessage(hChildList, LB_SETITEMHEIGHT, 0, 28); // 0代表所有项，28是行高(像素)，可自己调整
		//// 5. 准备填充下一级数据
		std::vector<const TCHAR*> vecChildData = { TEXT("text"), TEXT("text"), TEXT("text"), TEXT("text") };
		for (size_t i = 0; i < vecChildData.size(); i++)
			SendMessage(hChildList, LB_ADDSTRING, 0, (LPARAM)vecChildData[i]);
		
		//SWP_NOMOVE
		// 4. 子类化绑定通用Proc（关键：让ListBox能自绘）
		//if (g_pOldListBoxProc == NULL)
		//{
		//	//把系统默认给 ListBox 的「消息处理函数」，替换成你自己写的 MyGlobalListBoxProc；
		//	//同时把「被换掉的系统原函数地址」保存到 g_pOldListBoxProc 变量里，方便后续调用。
		//	g_pOldListBoxProc = (WNDPROC)SetWindowLongPtr(hChildList, GWLP_WNDPROC, (LONG_PTR)SubListBoxProc);
		//}
		//else
		//{
		//	SetWindowLongPtr(hChildList, GWLP_WNDPROC, (LONG_PTR)SubListBoxProc);
		//}

		// 5. 强制刷新，避免白屏
		InvalidateRect(hChildList, NULL, TRUE); // 标记整个控件为「无效区域」，需要重绘
		UpdateWindow(hChildList);               // 立即发送WM_PAINT → 进而触发WM_DRAWITEM
	}
}

VOID CreateSubWindow(HWND hSenderList, HWND hWnd)
{
	RECT rcListBox;
	GetWindowRect(hSenderList, &rcListBox);

	HWND hWindow2 = CreateWindowEx(
		WS_EX_TOOLWINDOW,          // 常用于浮动工具窗口
		L"Window2Class",//L""
		L"test",
		WS_POPUP | WS_VISIBLE,
		rcListBox.left - 3 * g_nListSpace - g_nListWidth, rcListBox.top, g_nListWidth, g_nListHeight,
		nullptr,                   // ❗没有 parent
		nullptr,
		hInst,
		nullptr
	);

	HWND hListBox2 = CreateWindowEx(
		0,
		WC_LISTBOX,
		TEXT(""),
		WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
		0, 0, 50, 50, // 铺满整个窗口2
		hWindow2, (HMENU)3003, hInst, NULL
	);
	if (hListBox2 == NULL) return;

	// 3. 设置ListBox2的行高（自绘必加，唯一的规则）
	SendMessage(hListBox2, LB_SETITEMHEIGHT, 0, 28);
	SendMessage(hListBox2, LB_RESETCONTENT, 0, 0); // 清空数据
	std::vector<const TCHAR*> vecChildData = { TEXT("text1"), TEXT("text2"), TEXT("text3"), TEXT("text4") };
	for (size_t i = 0; i < vecChildData.size(); i++)
	{
		SendMessage(hListBox2, LB_ADDSTRING, 0, (LPARAM)vecChildData[i]);
	}
	ShowWindow(hWindow2, SW_SHOW);
	UpdateWindow(hWindow2);
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
	static HWND hMainListBox = NULL;

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
		WNDCLASSEX wcCheck = { 0 };
		wcCheck.cbSize = sizeof(WNDCLASSEX);
		if (GetClassInfoEx(hInst, MY_LISTBOX_CLASS_NAME, &wcCheck))
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
			WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED,//  LBS_OWNERDRAWFIXED// 自定义绘制的最优样式组合（必记，你的需求专属）ListBox 的“绘制责任”已经完全交给你了，
			//如果你不处理 WM_DRAWITEM，系统就什么都不会画。
			20, 20, 200, 300,
			hWnd, (HMENU)ID_LISTBOX, hInst, NULL
		); 
		g_vListBoxHwnd.resize(5);
		g_vListBoxHwnd.at(0).push_back(hMainListBox);
		//g_vListBoxHwnd.push_back(hMainListBox);
		//set the height of item
		SendMessage(hMainListBox, LB_SETITEMHEIGHT, 0, 40);
		// 这里为什么正确：
		/*items[i] 指向静态区字符串
		LB_ADDSTRING 会 复制字符串 到 ListBox 内部
			无论你什么时候调用 LB_GETTEXT，都能取到* /
		//const WCHAR* items[] = { L"桌面" ,L"文档",L"下载",L"图片",L"视频",L"音乐",L"此电脑",L"回收站" };//
		/*const WCHAR* items[] = {
				TEXT("桌面"), TEXT("文档"), TEXT("下载"),
				TEXT("图片"), TEXT("视频"), TEXT("音乐"),
				TEXT("此电脑"), TEXT("回收站")
		};
		int count = sizeof(items) / sizeof(items[0]);
		for (int i = 0; i < count; i++) {
			SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)items[i]);
		}*/
		

		//这里为什么错误：
		//item.c_str() 返回的是 std::wstring 内部的 指针,LB_GETTEXT取指针的时候已经超出了作用域。后面取的时候其实只是取指针。
		//std::vector<std::wstring> vItem = { L"桌面", L"文档", L"下载", L"图片", L"视频", L"音乐", L"此电脑", L"回收站" };//
		//std::for_each(vItem.begin(), vItem.end(), [](const std::wstring& item) {
		//	SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)item.c_str());
		//	});
		//所以这里加个static即可
		static std::vector<std::wstring> vItem = { L"临时存放", L"优先", L"核心能力", L"核心能力但不那么好", L"非核心能力" , L"其它" };//
		std::for_each(vItem.begin(), vItem.end(), [](const std::wstring& item) {
			SendMessage(hMainListBox, LB_ADDSTRING, 0, (LPARAM)item.c_str());
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
		break;
	}
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
		// 只处理我们的ListBox控件
		//if (pDIS->CtlID != ID_LISTBOX || pDIS->itemID == LB_ERR) break;
		std::optional<int> nIndex = FindListBoxLevel(g_vListBoxHwnd, pDIS->hwndItem);
		if (!nIndex.has_value())
		{
			break;
		}

		if (hChildList == pDIS->hwndItem)
		{
			OutputDebugStringW(L"[bookcollector]test！\n");
		}

		if (0 == nIndex)//主ListBoxpDIS->hwndItem == g_hMainListBox
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
			int len = SendMessage(pDIS->hwndItem, LB_GETTEXTLEN, nItem, 0);
			std::wstring wBuff(256, L'\0');
			//注意这里的错误
			//WCHAR szBuff[256] = { 0 };//这里传递一个数组，
			//SendMessage(pDIS->hwndItem, LB_GETTEXT, nItem, (LPARAM)szBuff);
			//WCHAR szBuff[256] = { 0 };

			wchar_t* p = NULL;//传递一个指针，然后拿到指向那些字符串常量的地址
			SendMessage(pDIS->hwndItem, LB_GETTEXT, nItem, (LPARAM)&p);//传递&p更改的是p的值，那么传szBuff,更改的不就是*szBuff的值了么
			//wBuff.resize(wcslen(wBuff.data())); // 去掉多余 '\0'
			SetBkMode(hdc, TRANSPARENT);          // 文字背景透明，必加
			SetTextColor(hdc, RGB(20, 20, 20));   // 深灰色文字，比纯黑更柔和
			// 文字绘制区域：向右偏移35像素，避开图标
			RECT rcText = rc;
			rcText.left += 35;
			DrawText(hdc, p, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
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
			
		//有一种情况鼠标移动出窗口不会隐藏，那就是：鼠标沿着主窗口的右边框往上或往下移动
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
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == LBN_SELCHANGE)
		{
			// 选中项改变
			int index = (int)SendMessage(hMainListBox, LB_GETCURSEL, 0, 0);
			HWND hSenderList = (HWND)lParam;
			//TODO:这里查下发送消息的HWND属于哪一级别
			/*if (index != LB_ERR)
			{
				CreateSubListBox(hSenderList, hWnd);
				break;
			}*/
			if (index != LB_ERR)
			{
				CreateSubWindow(hSenderList, hWnd);
				break;
			}
		}
		else if (HIWORD(wParam) == LBN_DBLCLK)
		{
			// 双击
		}

		// 分析菜单选择:
		switch (LOWORD(wParam))
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
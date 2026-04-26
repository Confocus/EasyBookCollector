// EasyBookCollectorGUI.cpp : 定义应用程序的入口点。
//

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
#include "../public/PipeMgr.h"
#include <shellapi.h>
#include "ListViewMgr.h"

#define PIPE_NAME_BOOKMARK_COM	L"\\\\.\\pipe\\BookmarkTransPipe"
#define EVENT_NAME_RECV_CMD	L"{31E3A6F1-105A-45D9-8E73-79CE24064F5C}\ConnectPipe"

#define MAX_LOADSTRING 100
const int HOVER_TIME = 300;
BOOL g_bIsTrackRegistered = FALSE;

CMainWindowActions g_MainWndActions;
CListBoxWndManager g_ListBoxWndMgr;
CPipeMgr::CPipeServer g_PipeMgr;
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

//todo:完善并理解代码
//todo:验证切换成四屏的功能
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

CListViewMgr g_ListViewMgr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_ERASEBKGND:
		return TRUE; // 不擦除背景，自己在WM_PAINT里绘制，解决了拖拽Splitter时ListView边缘闪烁的问题
	case WM_CREATE: 
	{
		g_ListViewMgr.InitDoubleListViewAndLoadData(hWnd);
		break;
	}
	case WM_SIZING:
	{
		// 用户开始拖动边框调整大小
		g_ListViewMgr.SetBorderDraggedStatus(TRUE);
		break;
	}
	case WM_EXITSIZEMOVE:
	{
		// 用户结束了拖动（鼠标松开或按回车）
		g_ListViewMgr.SetBorderDraggedStatus(FALSE);
		break;
	}
	case WM_SIZE: 
	{
		g_ListViewMgr.DragSplitterAndRefreshAllListView(hWnd);
		break;
	}

	// 处理拆分条拖动
	case WM_LBUTTONDOWN: 
	{
		g_ListViewMgr.PressSplitter(hWnd, msg, wParam, lParam);
		break;
	}
	case WM_LBUTTONUP:
	{
		g_ListViewMgr.ReleaseSplitter(hWnd, msg, wParam, lParam);
		break;
	}
	case WM_MOUSEMOVE: 
	{
		g_ListViewMgr.DragSplitterAndSendMessage(hWnd, msg, wParam, lParam);
		break;
	}
	case WM_NOTIFY: // 处理ListView双击（核心：进入虚拟文件夹）
	{
		g_ListViewMgr.VisitListViewFolder(hWnd, msg, wParam, lParam);
		break;
	}

	// 处理Backspace返回上一级
	case WM_KEYDOWN: 
	{
		// 原有Backspace返回上一级逻辑不变...
			// F12切换双/四面板
		if (wParam == VK_F11) {
			//todo:如何在切换时仍保留当前的访问状态
			g_ListViewMgr.TogglePanelMode(hWnd);
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

	case WM_DESTROY: {
		g_ListViewMgr.Destory();
		PostQuitMessage(0);
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

unsigned __stdcall CommunicateWithDaemon(void* param)
{
	//todo:创建一个双向管道，用来发送命令和接收书签内容
	// todo：通知有新命令，需要接收解析命令，并通过管道发送命令
	// todo：接收书签，如果用一个管道就不需要再通知了，因为通知接收书签时肯定已经创建好管道了
	// todo：确定收发命令的格式
	//通知可以去解析数据了（但是现在）
	HANDLE hRecvCmdEvent = CreateEvent(
		NULL,
		FALSE,
		FALSE,
		EVENT_NAME_RECV_CMD
	);

	if (hRecvCmdEvent == NULL)
	{
		return 0;
	}

	SetEvent(hRecvCmdEvent);

	HANDLE hPipe = CreateNamedPipe(
		PIPE_NAME_BOOKMARK_COM,
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1, 4096, 4096, 0, nullptr);
	
	BOOL connected = ConnectNamedPipe(hPipe, NULL);

	if (!connected)
	{
		CloseHandle(hPipe);
		return 0;
	}

	//todo:通知管道创建好，可以接受书签内容了
	char buf[4096]{};
	DWORD readLen = 0;
	//todo:等待

	while (true)
	{
		// 无数据就阻塞等待，零轮询、低CPU
		BOOL ok = ReadFile(hPipe, buf, 4095, &readLen, nullptr);
		if (!ok || readLen == 0)
		{
			// 客户端断开 / 出错，退出循环
			break;
		}

		buf[readLen] = '\0';
		// 处理收到的 RPC 数据
		printf("收到: %s\n", buf);

		// 可选：WriteFile 回复数据
	}

	CloseHandle(hPipe);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) 
{
	InitCommonControls();
	if (!MyRegisterClass(hInstance)) return FALSE;

	HWND hWnd = CreateWindowW(L"VirtualFolderDemo", L"书籍目录保存",
		WS_OVERLAPPEDWINDOW , CW_USEDEFAULT, CW_USEDEFAULT, g_ListViewMgr.GetInitMainWndWidth(), 600,//| WS_CLIPCHILDREN
		NULL, NULL, hInstance, NULL);
	if (!hWnd) return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, CommunicateWithDaemon, (void*)NULL, 0, 0);

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



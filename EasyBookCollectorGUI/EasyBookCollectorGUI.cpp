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
#include <thread>
#include "EasyBookCollectorGUI.h"
#include "MainWindowActions.h"
#include "ListBoxWndManager.h"
#include "../public/PipeMgr.h"
#include <shellapi.h>

#define MAX_LOADSTRING 100
const int HOVER_TIME = 300;
BOOL g_bIsTrackRegistered = FALSE;
BOOL g_bDragging = FALSE;

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

#define MAX_NAME_LEN 256

// ===================== 自定义数据结构（模拟虚拟文件夹/数据项） =====================
// 虚拟节点：表示文件夹/数据项
typedef struct {
	unsigned int nID;             // 唯一ID
	BOOL bIsFolder;     // 是否是文件夹
	WCHAR szName[MAX_NAME_LEN];    // 显示名称
	unsigned int nParentId;      // 父节点ID（-1表示根节点）
	// 自定义数据：比如数据库ID、内容描述等
	int db_id;          // 模拟数据库ID
	WCHAR szDesc[512];    // 模拟自定义描述
} ItemNode;

// 模拟自定义数据（替代本地文件/数据库）
ItemNode g_szTestNode[] = {
	// 根节点（parent_id=-1）
	{1, true, L"我的图书分类", -1, 0, L""},
	{2, true, L"我的收藏夹", -1, 0, L""},
	{3, false, L"临时笔记.txt", -1, 1001, L"这是自定义数据项，不是文件"},
	// 图书分类的子节点（parent_id=1）
	{4, true, L"编程类", 1, 0, L""},
	{5, true, L"小说类", 1, 0, L""},
	{6, false, L"Python实战.md", 1, 1002, L"Python入门教程，自定义数据"},
	// 编程类的子节点（parent_id=4）
	{7, false, L"Java核心技术.md", 4, 1003, L"Java进阶内容，自定义数据"},
	{8, false, L"C++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
	{9, false, L"D++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
	{10, false, L"E++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
	{11, false, L"F++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
	{12, false, L"G++ Primer.md", 4, 1004, L"C++基础，自定义数据"},
	{13, false, L"H++ Primer.md", 4, 1004, L"C++基础，自定义数据"},

};
unsigned int g_nNodeCount = sizeof(g_szTestNode) / sizeof(ItemNode);

HWND g_hLeftListView, g_hRightListView;   // 左右面板ListView
HWND g_hSplitter;                 // 拆分条
const unsigned int g_nSplitterX = 400;           // 拆分条位置
const unsigned int g_nDefaultSubWindowWidth = 400;
const unsigned int g_nDefaultSplitterWidth = 10;
int g_nSplitterPos = 0;
// 当前层级：记录每个面板的当前父节点ID（模拟“当前目录”）
int g_left_current_parent = -1;
int g_right_current_parent = -1;
// 图标列表（文件夹/文件图标）
HIMAGELIST g_hImageList = NULL;
BOOL g_bInit = TRUE;

// ===================== 工具函数 =====================
// 初始化图标列表（文件夹+文件图标）
BOOL InitImageList() {
	g_hImageList = ImageList_Create(
		16, 16, //创建一个 16×16 像素
		ILC_COLOR32 | ILC_MASK,//32 位真彩色带透明通道
		2, //初始可存 2 个图标
		0);//无自动扩容 的图像列表，用于存放你后续添加的文件夹 / 文件通用图标。
	if (g_hImageList == NULL) {
		//wprintf(L"ImageList_Create 失败！错误码：%d\n", GetLastError());
		return FALSE;
	}

	SHFILEINFOW sfi = { 0 };

	// 添加文件夹图标
	SHGetFileInfoW(
		L"D:\\Tools",//todo:这里改下获取图标的方式； 
		FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	//把 SHGetFileInfoW 返回的文件夹小图标句柄（sfi.hIcon）添加到图像列表中；
	//作用：后续你可以通过图像列表的索引（比如 0）来使用这个文件夹图标（比如给 ListView / TreeView 控件设置图标）。
	int nFolderIconIndex = ImageList_AddIcon(g_hImageList, sfi.hIcon);
	DestroyIcon(sfi.hIcon);

	// 添加文件图标
	SHGetFileInfoW(L"*.txt", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	nFolderIconIndex = ImageList_AddIcon(g_hImageList, sfi.hIcon);
	DestroyIcon(sfi.hIcon);

	return TRUE;
}

// 加载指定父节点下的所有虚拟节点到ListView
void LoadVirtualFolder(HWND hList, int parent_id)
{
	// 清空列表
	ListView_DeleteAllItems(hList);

	// 遍历自定义数据，加载对应节点
	for (int i = 0; i < g_nNodeCount; i++)
	{
		ItemNode* node = &g_szTestNode[i];
		if (node->nParentId != parent_id) continue;

		// 插入ListView项
		LVITEMW lvi = { 0 };
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
		lvi.iItem = ListView_GetItemCount(hList);
		lvi.pszText = node->szName;
		// 图标：0=文件夹，1=文件
		lvi.iImage = node->bIsFolder ? 0 : 1;
		// 绑定自定义节点ID（关键：双击时识别是哪个节点）
		lvi.lParam = node->nID;
		ListView_InsertItem(hList, &lvi);
	}
}

// 根据节点ID查找虚拟节点
ItemNode* FindVirtualFoldNode(int node_id) {
	for (int i = 0; i < g_nNodeCount; i++) {
		if (g_szTestNode[i].nID == node_id) {
			return &g_szTestNode[i];
		}
	}
	return NULL;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg)
	{
	case WM_CREATE: 
	{
		// 初始化图标列表
		InitImageList();
		RECT rcParent;
		GetClientRect(hWnd, &rcParent);
		int nListViewHeight = rcParent.bottom - rcParent.top; // 父窗口完整高度

		// 创建拆分条
		//注意，这里的CreateWindows时输入的坐标会被WM_SIZE是MoveWindow的坐标覆盖掉
		//这里不用计算了
		g_hSplitter = CreateWindowW(L"STATIC", L"",
			WS_CHILD | WS_VISIBLE | SS_ETCHEDVERT,
			g_nSplitterX, 0, g_nDefaultSplitterWidth, nListViewHeight, hWnd, NULL, GetModuleHandle(NULL), NULL);

		// 创建左面板ListView
		g_hLeftListView = CreateWindowW(WC_LISTVIEWW, L"",
			WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS  | WS_BORDER,
			0, 0, g_nDefaultSubWindowWidth, nListViewHeight, hWnd, NULL, GetModuleHandle(NULL), NULL);

		ListView_SetImageList(g_hLeftListView, g_hImageList, LVSIL_SMALL);

		//如果在CreateWindowW时使用了属性LVS_SMALLICON，后面的列就显示不出来了
		LVCOLUMNW lvc = { 0 };
		lvc.mask = LVCF_TEXT | LVCF_WIDTH;
		lvc.pszText = const_cast<LPWSTR>(L"名称");
		lvc.cx = 100;
		ListView_InsertColumn(g_hLeftListView, 0, &lvc);
		lvc.pszText = const_cast<LPWSTR>(L"修改时间");
		lvc.cx = 50;
		ListView_InsertColumn(g_hLeftListView, 1, &lvc);
		
		// 初始化左面板列（仅显示名称，模拟文件夹列表）
		LoadVirtualFolder(g_hLeftListView, g_left_current_parent);
		
		// 创建右面板ListView
		g_hRightListView = CreateWindowW(WC_LISTVIEWW, L"",
			WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
			g_nSplitterX + g_nDefaultSplitterWidth, 0, g_nDefaultSubWindowWidth, nListViewHeight, hWnd, NULL, GetModuleHandle(NULL), NULL);
		ListView_SetImageList(g_hRightListView, g_hImageList, LVSIL_SMALL);
		
		lvc.mask = LVCF_TEXT | LVCF_WIDTH;
		lvc.pszText = const_cast<LPWSTR>(L"名称");
		lvc.cx = 100;
		ListView_InsertColumn(g_hRightListView, 0, &lvc);
		lvc.pszText = const_cast<LPWSTR>(L"修改时间");
		lvc.cx = 50;
		ListView_InsertColumn(g_hRightListView, 1, &lvc);
		LoadVirtualFolder(g_hRightListView, g_right_current_parent);

		break;
	}

	case WM_SIZE: 
	{
		if (g_bInit)//第一次初始化的时候
		{
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			int nSplitterWidth = g_nDefaultSplitterWidth;
			int nWidth = (rcClient.right - rcClient.left - g_nDefaultSplitterWidth) / 2;
			MoveWindow(g_hLeftListView, 0, 0, nWidth, rcClient.bottom, TRUE);
			MoveWindow(g_hSplitter, nWidth, 0, nSplitterWidth, rcClient.bottom, TRUE);
			MoveWindow(g_hRightListView, nWidth + nSplitterWidth, 0, nWidth, rcClient.bottom, TRUE);
			g_bInit = FALSE;
		}
		

		if (g_bDragging)
		{
			// 获取父窗口客户区尺寸
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);
			int nClientWidth = rcClient.right;
			int nClientHeight = rcClient.bottom;

			// 调整左面板尺寸
			SetWindowPos(g_hLeftListView, NULL,
				0, 0, g_nSplitterPos, nClientHeight,
				SWP_NOZORDER | SWP_NOACTIVATE);

			// 调整分隔条尺寸
			SetWindowPos(g_hSplitter, NULL,
				g_nSplitterPos, 0, g_nDefaultSplitterWidth, nClientHeight,
				SWP_NOZORDER | SWP_NOACTIVATE);

			// 调整右面板尺寸
			SetWindowPos(g_hRightListView, NULL,
				g_nSplitterPos + g_nDefaultSplitterWidth, 0,
				nClientWidth - g_nSplitterPos - g_nDefaultSplitterWidth, nClientHeight,
				SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;
	}

				// 处理拆分条拖动
	case WM_LBUTTONDOWN: {
		RECT rc;
		GetWindowRect(g_hSplitter, &rc);
		RECT rcSplitter;
		GetWindowRect(g_hSplitter, &rcSplitter);
		POINT pt = { LOWORD(lParam), HIWORD(lParam) };
		ClientToScreen(hWnd, &pt); // 转换为屏幕坐标

		if (PtInRect(&rcSplitter, pt)) 
		{
			g_bDragging = TRUE;
			// 设置鼠标捕获，确保拖动时能接收鼠标消息
			SetCapture(hWnd);
			// 改变鼠标光标为左右箭头
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		}
		break;
	}
	case WM_MOUSEMOVE: 
	{
		if (g_bDragging) {
			// 获取鼠标当前位置（客户区坐标）
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			// 限制分隔条的拖动范围（避免拖出边界）
			//int nMinPos = 50;  // 左面板最小宽度
			//int nMaxPos = rcClient.right - 100; // 右面板最小宽度
			//g_nSplitterPos = max(nMinPos, min(pt.x, nMaxPos));
			g_nSplitterPos = pt.x;
			// 立即刷新布局（触发WM_SIZE）
			SendMessage(hWnd, WM_SIZE, 0, 0);
			// 保持鼠标光标样式
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		}
		break;
	}
	case WM_LBUTTONUP: 
	{
		if (g_bDragging)
		{
			ReleaseCapture();
			g_bDragging = FALSE;
		}
		break;
	}

					 // 处理ListView双击（核心：进入虚拟文件夹）
	case WM_NOTIFY: {
		NMHDR* pNMHDR = (NMHDR*)lParam;
		// 左面板双击
		if (pNMHDR->hwndFrom == g_hLeftListView && pNMHDR->code == NM_DBLCLK) {
			LPNMITEMACTIVATE pNMItem = (LPNMITEMACTIVATE)lParam;
			int node_id = (int)pNMItem->iItem != -1 ? ListView_GetItem(g_hLeftListView, pNMItem->iItem) : -1;
			ItemNode* node = FindVirtualFoldNode(node_id);
			if (node && node->bIsFolder) {
				// 进入文件夹：更新当前父节点，重新加载
				g_left_current_parent = node->nID;
				LoadVirtualFolder(g_hLeftListView, g_left_current_parent);
			}
			else if (node && !node->bIsFolder) {
				// 点击数据项：显示自定义数据
				WCHAR msg[512];
				wsprintfW(msg, L"自定义数据：\n名称：%s\n数据库ID：%d\n描述：%s",
					node->szName, node->db_id, node->szDesc);
				MessageBoxW(hWnd, msg, L"自定义数据详情", MB_OK);
			}
		}
		// 右面板双击
		if (pNMHDR->hwndFrom == g_hRightListView && pNMHDR->code == NM_DBLCLK) {
			LPNMITEMACTIVATE pNMItem = (LPNMITEMACTIVATE)lParam;
			int node_id = (int)pNMItem->iItem != -1 ? ListView_GetItem(g_hRightListView, pNMItem->iItem) : -1;
			ItemNode* node = FindVirtualFoldNode(node_id);
			if (node && node->bIsFolder) {
				g_right_current_parent = node->nID;
				LoadVirtualFolder(g_hRightListView, g_right_current_parent);
			}
			else if (node && !node->bIsFolder) {
				WCHAR msg[512];
				wsprintfW(msg, L"自定义数据：\n名称：%s\n数据库ID：%d\n描述：%s",
					node->szName, node->db_id, node->szDesc);
				MessageBoxW(hWnd, msg, L"自定义数据详情", MB_OK);
			}
		}
		break;
	}

				  // 处理Backspace返回上一级
	case WM_KEYDOWN: {
		if (wParam == VK_BACK) {
			// 判断当前激活的面板
			HWND hFocus = GetFocus();
			if (hFocus == g_hLeftListView) {
				// 返回上一级：找到当前父节点的父节点
				ItemNode* curr_parent = FindVirtualFoldNode(g_left_current_parent);
				g_left_current_parent = curr_parent ? curr_parent->nParentId : -1;
				LoadVirtualFolder(g_hLeftListView, g_left_current_parent);
			}
			else if (hFocus == g_hRightListView) {
				ItemNode* curr_parent = FindVirtualFoldNode(g_right_current_parent);
				g_right_current_parent = curr_parent ? curr_parent->nParentId : -1;
				LoadVirtualFolder(g_hRightListView, g_right_current_parent);
			}
		}
		break;
	}

	case WM_DESTROY: {
		ImageList_Destroy(g_hImageList);
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	InitCommonControls();
	if (!MyRegisterClass(hInstance)) return FALSE;

	HWND hWnd = CreateWindowW(L"VirtualFolderDemo", L"书籍目录保存",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 2 * g_nDefaultSubWindowWidth + g_nDefaultSplitterWidth, 600,
		NULL, NULL, hInstance, NULL);
	if (!hWnd) return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return (int)msg.wParam;
}

// 辅助函数：获取客户区宽/高
int GetClientRectWidth(HWND hWnd) {
	RECT rc; GetClientRect(hWnd, &rc); return rc.right - rc.left;
}
int GetClientRectHeight(HWND hWnd) {
	RECT rc; GetClientRect(hWnd, &rc); return rc.bottom - rc.top;
}



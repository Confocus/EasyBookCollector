#include "ListViewMgr.h"
#include <optional>
#include <cstdlib>
#include <cmath>    // 用于 double、float、long double 类型的 abs
#include "DropTarget.h"
#include "BookMarksNode.h"
#include "CBookMarksMgr.h"
//起初把这个声明包含在ListViewMgr.h文件里会报错
//ListViewMgr.h 被包含时，CPipeCommManager 类型还没有定义完成。
//也就是说：
//#include "PipeCommManager.h"
//extern CPipeCommManager PipeCommMgr;
//放在ListViewMgr.cpp时：
//include 顺序刚好没问题
//但放在ListViewMgr.h时：
//触发了头文件循环依赖
#include "PipeMessageHandler.h"
extern CPipeMessageHandler g_PipeCommMgr;

// 模拟自定义数据（替代本地文件/数据库）
ItemNode g_szTestNode[] = 
{
	// 根节点（parent_id=-1）
	{1, true, L"我的图书分类", -1, 0, L""},
	{2, true, L"我的收藏夹", -1, 0, L""},
	{3, false, L"临时笔记.txt", -1, 1001, L"这是自定义数据项，不是文件"},
	// 图书分类的子节点（parent_id=1）
	{4, true, L"编程类", 1, 0, L""},
	{5, true, L"小说类", 1, 0, L""},
	{6, false, L"Python实战.md", 2, 1002, L"Python入门教程，自定义数据"},
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
WNDPROC g_OldListViewProc = NULL;

//注意：const 全局变量默认是 internal linkage（内部链接）
// 当前层级：记录每个面板的当前父节点ID（模拟“当前目录”）
int g_right_current_parent = -1;
// 图标列表（文件夹/文件图标）

LRESULT CALLBACK ListViewSubProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// 核心：ListView自身也不擦除背景
	if (uMsg == WM_ERASEBKGND)
	{
		return TRUE; // 和主窗口逻辑一致，跳过擦除
	}
	// 其他消息走原始逻辑，不影响ListView功能
	return CallWindowProc(g_OldListViewProc, hWnd, uMsg, wParam, lParam);
}

CListViewMgr::CListViewMgr():
	m_hImageList(NULL),
	m_bInit(TRUE),
	m_eDraggingType(DRAG_TYPE_STOP),
	m_PanelMode(PANEL_MODE_DOUBLE),
	m_hVerticalSplitter(NULL),
	m_hHorizontalSplitter(NULL),
	m_hLeftListView(NULL),
	m_hRightListView(NULL),
	m_hTopRightListView(NULL),
	m_hTopLeftListView(NULL),
	m_nInitListViewHeight(0),
	m_nInitListViewWidth(0),
	m_nInitMainWndWidth(810),
	m_nInitSplitterWidth(10),
	m_nCurrentVerticalSplitterX(0),
	m_nCurrentHorizontalSplitterY(0),
	m_nLeftCurrentParent(-1),
	m_bIsBorderDragged(FALSE)
{
}

CListViewMgr::~CListViewMgr()
{

}

BOOL CListViewMgr::InitDoubleListViewAndLoadData(HWND hWnd)
{
	SHFILEINFOW sfi = { 0 };

	HIMAGELIST hSysImageList = (HIMAGELIST)SHGetFileInfoW(
		L"C:\\Windows",
		FILE_ATTRIBUTE_DIRECTORY,
		&sfi,
		sizeof(sfi),
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON
	);

	//m_vecNodes = CBookMarksMgr::instance().GetAllBookMarksNodes();
	//InitImageList();
	// 初始化图标列表
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);
	m_nInitListViewHeight = rcClient.bottom - rcClient.top; // 父窗口完整高度
	m_nInitListViewWidth = (rcClient.right - rcClient.left - m_nInitSplitterWidth) / 2;
	m_nInitSplitterX = m_nInitListViewWidth;
	m_nCurrentVerticalSplitterX = m_nInitSplitterX;
	m_nInitSplitterY = (m_nInitListViewHeight - m_nInitSplitterWidth) / 2;
	m_nCurrentHorizontalSplitterY = m_nInitSplitterY;
	m_nLastSplitterX = m_nInitListViewWidth;
	//创建拆分条
	//注意，这里的CreateWindows时输入的坐标会被WM_SIZE是MoveWindow的坐标覆盖掉
	//这里不用计算了
	m_hVerticalSplitter = CreateWindowW(L"STATIC", L"",
		WS_CHILD | WS_VISIBLE,// | SS_ETCHEDVERT
		m_nInitSplitterX, 0, m_nInitSplitterWidth, m_nInitListViewHeight, hWnd, NULL, GetModuleHandle(NULL), NULL);

	// 创建左面板ListView
	m_hLeftListView = CreateWindowW(WC_LISTVIEWW, L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,//| WS_BORDER | WS_CLIPCHILDREN
		0, 0, m_nInitListViewWidth, m_nInitListViewHeight, hWnd, NULL, GetModuleHandle(NULL), NULL);
	ListView_SetExtendedListViewStyle(m_hLeftListView, LVS_EX_INFOTIP);
	//注册以响应拖拽事件
	RegisterDragDrop(m_hLeftListView, (IDropTarget*)new CDropTarget());
	DragAcceptFiles(m_hLeftListView, TRUE);


	ListView_SetImageList(m_hLeftListView, m_hImageList, LVSIL_SMALL);
	ListViewInsertColumn(m_hLeftListView);
	// 初始化左面板列（仅显示名称，模拟文件夹列表）
	LoadVirtualFolders(m_hLeftListView, m_nLeftCurrentParent);
	ListView_SetImageList(m_hLeftListView, hSysImageList, LVSIL_SMALL);

	//// 创建右面板ListView
	m_hRightListView = CreateWindowW(WC_LISTVIEWW, L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
		m_nInitSplitterX + m_nInitSplitterWidth, 0, m_nInitListViewWidth, m_nInitListViewHeight, hWnd, NULL, GetModuleHandle(NULL), NULL);
	ListView_SetExtendedListViewStyle(m_hRightListView, LVS_EX_INFOTIP);
	RegisterDragDrop(m_hRightListView, (IDropTarget*)new CDropTarget());
	DragAcceptFiles(m_hRightListView, TRUE);

	ListView_SetImageList(m_hRightListView, m_hImageList, LVSIL_SMALL);
	ListViewInsertColumn(m_hRightListView);
	LoadVirtualFolders(m_hRightListView, g_right_current_parent);
	ListView_SetImageList(m_hRightListView, hSysImageList, LVSIL_SMALL);
	//m_hToolTip = CreateWindow(
	//	TOOLTIPS_CLASS,
	//	NULL,
	//	WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
	//	CW_USEDEFAULT, CW_USEDEFAULT,
	//	CW_USEDEFAULT, CW_USEDEFAULT,
	//	hWnd, NULL, GetModuleHandle(NULL), NULL
	//);

	//// 绑定到 ListView
	//TOOLINFO ti = { 0 };
	//ti.cbSize = sizeof(TOOLINFO);
	//ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	//ti.hwnd = m_hLeftListView;
	//ti.uId = (UINT_PTR)m_hLeftListView;
	//ti.lpszText = LPSTR_TEXTCALLBACK;  // 动态获取文字

	//SendMessage(m_hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
	//SendMessage(m_hToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, 200);  // 悬浮多久后弹出

	return TRUE;
}

VOID CListViewMgr::AdjustDoubleListView(HWND hWnd, unsigned int nMainWndWidth, unsigned int nCurrentVerticalSplitterX, unsigned int nListViewHeight, unsigned int nSplitterWidth)//
{
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	m_nLastSplitterX = nCurrentVerticalSplitterX;
	// 调整左面板尺寸
	SetWindowPos(m_hLeftListView, NULL,
		0,
		0,
		nCurrentVerticalSplitterX,
		nListViewHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 调整分隔条尺寸
	SetWindowPos(m_hVerticalSplitter, NULL,
		nCurrentVerticalSplitterX,
		0,
		nSplitterWidth,
		nListViewHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 调整右面板尺寸
	SetWindowPos(m_hRightListView, NULL,
		nCurrentVerticalSplitterX + nSplitterWidth,
		0,
		nMainWndWidth - nCurrentVerticalSplitterX - nSplitterWidth,//rcClient.right - m_nCurrentSplitterX - m_nInitSplitterWidth,
		nListViewHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);

	//不刷新右侧窗口，会有拖拽的痕迹
	RECT rcInvalid = {
		nCurrentVerticalSplitterX + nSplitterWidth, // 左边界：取新旧X的最小值 + m_nInitSplitterWidth
		0,                                 // 上边界：顶部
		//rcClient.right会闪，改成m_nCurrentSplitterX + 2 * m_nInitSplitterWidth也会局部闪烁
		nCurrentVerticalSplitterX + 2 * nSplitterWidth, // 右边界：取新旧X的最大值+拆分条宽度（5）
		nListViewHeight                    // 下边界：底部
	};

	//不加Invalidate和Update就会有多余的一些颜色溢出Splitter
	//但是加上之后会闪烁，第三格参数改为FALSE就好了
	InvalidateRect(hWnd, &rcInvalid, FALSE);
	UpdateWindow(hWnd); // 立即刷新，避免延迟
}

VOID CListViewMgr::AdjustQuadListView(HWND hWnd, 
	unsigned int nMainWndWidth, 
	unsigned int nCurrentVerticalSplitterX, 
	unsigned int nListViewHeight, 
	unsigned int nSplitterWidth,
	unsigned int nCurrentHorizontalSplitterY)
{
	// 调整垂直分隔条尺寸
	SetWindowPos(m_hVerticalSplitter, NULL,
		nCurrentVerticalSplitterX,
		0,
		nSplitterWidth,
		nListViewHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 调整水平分隔条尺寸
	SetWindowPos(m_hHorizontalSplitter, NULL,
		0,
		nCurrentHorizontalSplitterY,
		nMainWndWidth,
		nSplitterWidth,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 调整左上面板尺寸
	SetWindowPos(m_hTopLeftListView, NULL,
		0,
		0,
		nCurrentVerticalSplitterX,
		nCurrentHorizontalSplitterY,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 调整右上面板尺寸
	SetWindowPos(m_hTopRightListView, NULL,
		nCurrentVerticalSplitterX + nSplitterWidth,
		0,
		nMainWndWidth - nCurrentVerticalSplitterX - nSplitterWidth,
		nCurrentHorizontalSplitterY,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 调整左下面板尺寸
	SetWindowPos(m_hBottomLeftListView, NULL,
		0,
		nCurrentHorizontalSplitterY + nSplitterWidth,
		nCurrentVerticalSplitterX,
		nListViewHeight - nCurrentHorizontalSplitterY - nSplitterWidth,
		SWP_NOZORDER | SWP_NOACTIVATE);

	// 调整右下面板尺寸
	SetWindowPos(m_hBottomRightListView, NULL,
		nCurrentVerticalSplitterX + nSplitterWidth,
		nCurrentHorizontalSplitterY + nSplitterWidth,
		nMainWndWidth - nCurrentVerticalSplitterX - nSplitterWidth,
		nListViewHeight - nCurrentHorizontalSplitterY - nSplitterWidth,
		SWP_NOZORDER | SWP_NOACTIVATE);

	RECT rcInvalid;
	if (m_eDraggingType == DRAG_TYPE_VIRTICAL)
	{
		rcInvalid = {
		static_cast<long>(nCurrentVerticalSplitterX),
		0,
		static_cast<long>(nMainWndWidth),
		static_cast<long>(nListViewHeight)
		};
	}
	else if (m_eDraggingType == DRAG_TYPE_HORIZONTAL)
	{
		rcInvalid = {
		0,
		static_cast<long>(m_nCurrentHorizontalSplitterY + nSplitterWidth),
		static_cast<long>(nMainWndWidth),
		static_cast<long>(nListViewHeight)
		};
	}

	InvalidateRect(hWnd, &rcInvalid, FALSE);
	UpdateWindow(hWnd);
}

BOOL CListViewMgr::DragSplitterAndRefreshAllListView(HWND hWnd)
{
	// 获取父窗口客户区尺寸
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);
	//如果加了这句： && std::abs(static_cast<int>(m_nCurrentSplitterX - m_nLastSplitterX)) > 2 会出现一个问题
	//即实际上m_nCurrentSplitterX移动了一个单位比如到591，但是由于m_nCurrentSplitterX - m_nLastSplitterX并没有大于2，所以
	//纵向Splitter并没有移动到591，而右上右下ListView则是基于591计算的，所以就会比Splitter大一个单位
	if (m_PanelMode == PANEL_MODE_DOUBLE)
	{
		m_nLastSplitterX = m_nCurrentVerticalSplitterX;
		if (IsSplitterDragged())//如果是因为拖拽Splitter导致刷新重绘
		{
			AdjustDoubleListView(hWnd, rcClient.right, m_nCurrentVerticalSplitterX, m_nInitListViewHeight, m_nInitSplitterWidth);
		}
		else if (IsBorderDragged())
		{
			m_nCurrentVerticalSplitterX = static_cast<float>(rcClient.right) / static_cast<float>(m_nInitMainWndWidth) * m_nInitSplitterX;
			AdjustDoubleListView(hWnd, rcClient.right, m_nCurrentVerticalSplitterX, rcClient.bottom, m_nInitSplitterWidth);
		}
	}
	
	if (m_PanelMode == PANEL_MODE_QUAD)
	{
		if (IsSplitterDragged())//如果是因为拖拽Splitter导致刷新重绘
		{
			AdjustQuadListView(hWnd, rcClient.right, m_nCurrentVerticalSplitterX, m_nInitListViewHeight, m_nInitSplitterWidth, m_nCurrentHorizontalSplitterY);
		}
		else if (IsBorderDragged())
		{
			m_nCurrentVerticalSplitterX = static_cast<float>(rcClient.right) / static_cast<float>(m_nInitMainWndWidth) * m_nInitSplitterX;
			m_nCurrentHorizontalSplitterY = static_cast<float>(rcClient.bottom) / static_cast<float>(m_nInitListViewHeight) * m_nInitSplitterY;
			AdjustQuadListView(hWnd, rcClient.right, m_nCurrentVerticalSplitterX, rcClient.bottom, m_nInitSplitterWidth, m_nCurrentHorizontalSplitterY);
		}
	}

	return TRUE;
}

BOOL CListViewMgr::PressSplitter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT rcVerticalSplitter;
	GetWindowRect(m_hVerticalSplitter, &rcVerticalSplitter);
	RECT rcHorizontalSplitter;
	GetWindowRect(m_hHorizontalSplitter, &rcHorizontalSplitter);

	POINT pt = { LOWORD(lParam), HIWORD(lParam) };
	ClientToScreen(hWnd, &pt); // 转换为屏幕坐标

	//判断鼠标是否在Splitter里
	if (PtInRect(&rcVerticalSplitter, pt))
	{
		m_eDraggingType = DRAG_TYPE_VIRTICAL;
		// 设置鼠标捕获，确保拖动时能接收鼠标消息
		SetCapture(hWnd);
		// 改变鼠标光标为左右箭头
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));

		// 暂停左右 ListView 重绘（关键：拖拽中不绘制，无闪烁）
		/*if (m_hLeftListView) SendMessage(m_hLeftListView, WM_SETREDRAW, FALSE, 0);
		if (m_hRightListView) SendMessage(m_hRightListView, WM_SETREDRAW, FALSE, 0);*/
	}
	else if (PtInRect(&rcHorizontalSplitter, pt))
	{
		m_eDraggingType = DRAG_TYPE_HORIZONTAL;
		SetCapture(hWnd);
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
	}

	return TRUE;
}

BOOL CListViewMgr::ReleaseSplitter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (IsSplitterDragged())
	{
		ReleaseCapture();
		SetDraggingStopStatus();
	}

	return TRUE;
}

BOOL CListViewMgr::DragSplitterAndSendMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 获取鼠标当前位置（客户区坐标）
	POINT pt = { LOWORD(lParam), HIWORD(lParam) };
	// 限制分隔条的拖动范围（避免拖出边界）
	//int nMinPos = 50;  // 左面板最小宽度
	//int nMaxPos = rcClient.right - 100; // 右面板最小宽度
	//g_nSplitterPos = max(nMinPos, min(pt.x, nMaxPos));
	/*RECT rcSplitter;
	GetWindowRect(m_hVerticalSplitter, &rcSplitter);*/

	//如果是拖拽的垂直Splitter只更新x
	if (DRAG_TYPE_VIRTICAL == m_eDraggingType)
	{
		m_nCurrentVerticalSplitterX = pt.x;
		// 立即刷新布局（触发WM_SIZE）
		SendMessage(hWnd, WM_SIZE, 0, 0);
		// 保持鼠标光标样式
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
	}
	else if (DRAG_TYPE_HORIZONTAL == m_eDraggingType)
	{
		m_nCurrentHorizontalSplitterY = pt.y;
		SendMessage(hWnd, WM_SIZE, 0, 0);
		SetCursor(LoadCursor(NULL, IDC_SIZENS));
	}
	else if (m_bIsBorderDragged)
	{
		SendMessage(hWnd, WM_SIZE, 0, 0);
	}
	
	return TRUE;
}

BOOL CListViewMgr::IsInitStatus()
{
	return m_bInit;
}

BOOL CListViewMgr::IsSplitterDragged()
{
	return m_eDraggingType != DRAG_TYPE_STOP;
}

BOOL CListViewMgr::IsBorderDragged()
{
	return m_bIsBorderDragged;
}

VOID CListViewMgr::SetBorderDraggedStatus(BOOL bStatus)
{
	m_bIsBorderDragged = bStatus;
}

VOID CListViewMgr::SetDraggingStopStatus()
{
	m_eDraggingType = DRAG_TYPE_STOP;
}

VOID CListViewMgr::Destory()
{
	if (m_hLeftListView)
	{
		RevokeDragDrop(m_hLeftListView);
	}

	if (m_hRightListView)
	{
		RevokeDragDrop(m_hRightListView);
	}

	if (m_hTopLeftListView)
	{
		DestroyWindow(m_hTopLeftListView);
		m_hTopLeftListView = NULL; // 销毁后置空，避免野指针
	}

	if (m_hTopRightListView)
	{
		DestroyWindow(m_hTopRightListView);
		m_hTopRightListView = NULL; 
	}

	if (m_hBottomLeftListView)
	{
		DestroyWindow(m_hBottomLeftListView);
		m_hBottomLeftListView = NULL;
	}

	if (m_hBottomRightListView)
	{
		DestroyWindow(m_hBottomRightListView);
		m_hBottomRightListView = NULL;
	}

	if (m_hLeftListView)
	{
		DestroyWindow(m_hLeftListView);
		m_hLeftListView = NULL;
	}

	if (m_hRightListView)
	{
		DestroyWindow(m_hRightListView);
		m_hRightListView = NULL;
	}

	if (m_hVerticalSplitter)
	{
		DestroyWindow(m_hVerticalSplitter);
		m_hVerticalSplitter = NULL;
	}

	if (m_hHorizontalSplitter)
	{
		DestroyWindow(m_hHorizontalSplitter);
		m_hHorizontalSplitter = NULL;
	}
	ImageList_Destroy(m_hImageList);
}

BOOL CListViewMgr::TogglePanelMode(HWND hWnd)
{
	// 1. 切换模式
	m_PanelMode = (m_PanelMode == PANEL_MODE_DOUBLE) ? PANEL_MODE_QUAD : PANEL_MODE_DOUBLE;

	// 2. 获取主窗口客户区大小
	// 如果切换为四个ListView,调整布局
	if (m_PanelMode == PANEL_MODE_QUAD) 
	{
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		// ========== 切换为四面板 ==========
		// 显示水平拆分条（没有则创建）
		if (!m_hHorizontalSplitter) 
		{
			m_hHorizontalSplitter = CreateWindowW(L"STATIC", L"",
				WS_CHILD | WS_VISIBLE ,//| SS_ETCHEDHORZ默认固定高度 
				0, 
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2, 
				m_nInitListViewWidth * 2 + m_nInitSplitterWidth, 
				m_nInitSplitterWidth, 
				hWnd, 
				NULL, 
				GetModuleHandle(NULL), 
				NULL);
		}
		else 
		{
			SetWindowPos(m_hHorizontalSplitter, NULL,
				0,
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2,
				m_nInitListViewWidth * 2 + m_nInitSplitterWidth,
				m_nInitSplitterWidth,
				SWP_SHOWWINDOW);
		}
		// 创建/显示四面板的4个ListView（没有则创建）
		// 上左面板
		if (!m_hTopLeftListView) 
		{
			m_hTopLeftListView = CreateWindowW(WC_LISTVIEWW, L"",
				WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,//注意：这里如果加了 LVS_SMALLICON 就不显示列了
				0, 
				0, 
				m_nCurrentVerticalSplitterX,//m_nInitListViewWidth, 
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2, 
				hWnd, 
				NULL, 
				GetModuleHandle(NULL), 
				NULL);
			ListView_SetImageList(m_hTopLeftListView, m_hImageList, LVSIL_SMALL);
			ListViewInsertColumn(m_hTopLeftListView);

			LoadVirtualFolders(m_hTopLeftListView, -1);
		}
		else 
		{
			SetWindowPos(m_hTopLeftListView, NULL,
				0,
				0,
				m_nCurrentVerticalSplitterX,
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2,
				SWP_SHOWWINDOW);
		}

		// 上右面板
		if (!m_hTopRightListView) 
		{
			m_hTopRightListView = CreateWindowW(WC_LISTVIEWW, L"",
				WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS ,
				m_nCurrentVerticalSplitterX + m_nInitSplitterWidth,//m_nInitSplitterX + m_nInitSplitterWidth, 
				0, 
				m_nInitListViewWidth + (m_nInitSplitterX - m_nCurrentVerticalSplitterX), //rcClient.right - (m_nCurrentSplitterX + m_nInitSplitterWidth), //
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2, //(rcClient.bottom - m_nInitSplitterWidth) / 2, //
				hWnd, 
				NULL, 
				GetModuleHandle(NULL), 
				NULL);
			ListView_SetImageList(m_hTopRightListView, m_hImageList, LVSIL_SMALL);
			ListViewInsertColumn(m_hTopRightListView);

			LoadVirtualFolders(m_hTopRightListView, -1);
		}
		else 
		{
			SetWindowPos(m_hTopRightListView, NULL,
				m_nCurrentVerticalSplitterX + m_nInitSplitterWidth,
				0,
				m_nInitListViewWidth + (m_nInitSplitterX - m_nCurrentVerticalSplitterX),
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2,
				SWP_SHOWWINDOW);
		}

		//// 下左面板
		if (!m_hBottomLeftListView) 
		{
			m_hBottomLeftListView = CreateWindowW(WC_LISTVIEWW, L"",
				WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,//WS_BORDER
				0, 
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2 + m_nInitSplitterWidth, 
				m_nCurrentVerticalSplitterX,//m_nInitListViewWidth,
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2,
				hWnd, 
				NULL, 
				GetModuleHandle(NULL), 
				NULL);
			ListView_SetImageList(m_hBottomLeftListView, m_hImageList, LVSIL_SMALL);
			ListViewInsertColumn(m_hBottomLeftListView);

			LoadVirtualFolders(m_hBottomLeftListView, -1);
		}
		else 
		{
			SetWindowPos(m_hBottomLeftListView, NULL,
				0,
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2 + m_nInitSplitterWidth,
				m_nCurrentVerticalSplitterX,
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2,
				SWP_SHOWWINDOW);
		}

		// 下右面板
		if (!m_hBottomRightListView) 
		{
			m_hBottomRightListView = CreateWindowW(WC_LISTVIEWW, L"",
				WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS ,
				m_nCurrentVerticalSplitterX + m_nInitSplitterWidth, //m_nInitSplitterX + m_nInitSplitterWidth,
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2 + m_nInitSplitterWidth, 
				m_nInitListViewWidth + (m_nInitSplitterX - m_nCurrentVerticalSplitterX), //rcClient.right - (m_nCurrentSplitterX + m_nInitSplitterWidth),//
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2, //(rcClient.bottom - m_nInitSplitterWidth) / 2, //
				hWnd, 
				NULL, 
				GetModuleHandle(NULL), 
				NULL);
			ListView_SetImageList(m_hBottomRightListView, m_hImageList, LVSIL_SMALL);
			ListViewInsertColumn(m_hBottomRightListView);
			LoadVirtualFolders(m_hBottomRightListView, -1);
		}
		else 
		{
			SetWindowPos(m_hBottomRightListView, NULL,
				m_nCurrentVerticalSplitterX + m_nInitSplitterWidth,
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2 + m_nInitSplitterWidth,
				m_nInitListViewWidth + (m_nInitSplitterX - m_nCurrentVerticalSplitterX),
				(m_nInitListViewHeight - m_nInitSplitterWidth) / 2,
				SWP_SHOWWINDOW);
		}
		//ShowWindow(m_hVerticalSplitter, SW_SHOW);

		// 隐藏原来的双面板控件
		ShowWindow(m_hLeftListView, SW_HIDE);
		ShowWindow(m_hRightListView, SW_HIDE);
		/*RECT rcInvalid = {
		m_nInitSplitterX + m_nInitSplitterWidth,
		0,
		m_nInitSplitterX + 2 * m_nInitSplitterWidth,
		m_nInitListViewHeight
		};
		InvalidateRect(hWnd, &rcInvalid, FALSE);
		UpdateWindow(hWnd);*/
	}
	else 
	{
		// ========== 切换为双面板 ==========
		// 隐藏水平拆分条和四面板控件
		if (m_hHorizontalSplitter) ShowWindow(m_hHorizontalSplitter, SW_HIDE);
		if (m_hTopLeftListView) ShowWindow(m_hTopLeftListView, SW_HIDE);
		if (m_hTopRightListView) ShowWindow(m_hTopRightListView, SW_HIDE);
		if (m_hBottomLeftListView) ShowWindow(m_hBottomLeftListView, SW_HIDE);
		if (m_hBottomRightListView) ShowWindow(m_hBottomRightListView, SW_HIDE);

		// 显示原来的双面板控件并调整大小
		/*ShowWindow(m_hLeftListView, SW_SHOW);
		ShowWindow(m_hRightListView, SW_SHOW);*/
		SetWindowPos(m_hLeftListView, NULL,
			0,
			0,
			m_nCurrentVerticalSplitterX,
			m_nInitListViewHeight,
			SWP_SHOWWINDOW);
		SetWindowPos(m_hRightListView, NULL,
			m_nCurrentVerticalSplitterX + m_nInitSplitterWidth,
			0,
			m_nInitSplitterX + (m_nInitSplitterX - m_nCurrentVerticalSplitterX),
			m_nInitListViewHeight,
			SWP_SHOWWINDOW);
		
	}

	// 刷新垂直拆分条位置
	//MoveWindow(m_hVerticalSplitter, m_nInitSplitterX, 0, 5, nClientHeight, TRUE);
	return TRUE;
}

BOOL CListViewMgr::InitImageList() 
{
	
	
	
	//int cx = GetSystemMetrics(SM_CXSMICON);
	//int cy = GetSystemMetrics(SM_CYSMICON);

	//m_hImageList = ImageList_Create(
	//	16, 16, //创建一个 16×16 像素
	//	ILC_COLOR32 | ILC_MASK,//32 位真彩色带透明通道//
	//	2, //初始可存 2 个图标
	//	0);//无自动扩容 的图像列表，用于存放你后续添加的文件夹 / 文件通用图标。
	//if (m_hImageList == NULL) 
	//{
	//	return FALSE;
	//}

	//SHFILEINFOW sfi = { 0 };

	//// 添加文件夹图标
	////todo:有没有其它方式优化？
	//SHGetFileInfoW(
	//	L"D:\\Tools",//todo:这里改下获取图标的方式； 
	//	FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
	//	SHGFI_ICON | SHGSI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	////把 SHGetFileInfoW 返回的文件夹小图标句柄（sfi.hIcon）添加到图像列表中；
	////作用：后续你可以通过图像列表的索引（比如 0）来使用这个文件夹图标（比如给 ListView / TreeView 控件设置图标）。
	//int nFolderIconIndex = ImageList_AddIcon(m_hImageList, sfi.hIcon);
	////int nFolderIconIndex = ImageList_ReplaceIcon(m_hImageList, -1, sfi.hIcon);

	//DestroyIcon(sfi.hIcon);

	//// 添加文件图标
	////todo:有没有其它方式优化？
	//SHGetFileInfoW(L"*.txt", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
	//	SHGFI_ICON | SHGSI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	//nFolderIconIndex = ImageList_AddIcon(m_hImageList, sfi.hIcon);
	////nFolderIconIndex = ImageList_ReplaceIcon(m_hImageList, -1, sfi.hIcon);

	//DestroyIcon(sfi.hIcon);

	////todo:有没有其它方式优化？
	//SHGetFileInfoW(
	//	L"D:\\Tools",
	//	FILE_ATTRIBUTE_DIRECTORY,
	//	&sfi,
	//	sizeof(sfi),
	//	SHGFI_ICON | SHGSI_SMALLICON | SHGFI_OPENICON
	//);
	//nFolderIconIndex = ImageList_AddIcon(m_hImageList, sfi.hIcon);

	//DestroyIcon(sfi.hIcon);

	return TRUE;
}

// 加载指定父节点下的所有虚拟节点到ListView
void CListViewMgr::LoadVirtualFolders(HWND hList, int parent_id)
{
	ListView_DeleteAllItems(hList);
	if (parent_id != -1) // 如果不是根目录的时候，显示“返回上一级”
	{
		SHFILEINFOW sfi = { 0 };
		LVITEMW lvi = { 0 };
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT;
		lvi.iItem = 0;
		lvi.pszText = const_cast<WCHAR*>(L"返回上一级...");
		lvi.iImage = 2;
		lvi.lParam = ID_BACK_TO_PARENT; // 绑定专属ID，用于识别点击
		lvi.iIndent = 1; // 无缩进（根级显示）
		//lviBack.iSubItem = 0;
		SHGetFileInfoW(
			L"D:\\Tools",
			FILE_ATTRIBUTE_NORMAL,
			&sfi,
			sizeof(sfi),
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON
		);
		lvi.iImage = sfi.iIcon;
		ListView_InsertItem(hList, &lvi);
	}

	const std::vector<CBookMarksNode>& vecNodes = CBookMarksMgr::instance().GetAllBookMarksNodes();
	const unsigned& uNodeCount = CBookMarksMgr::instance().GetBookMarksCnt();
	for (unsigned int i = 0; i < uNodeCount; i++)
	{
		if (vecNodes[i].m_nFatherNum != parent_id)
		{
			continue;
		}
		SHFILEINFOW sfi = { 0 };
		// 插入ListView项
		LVITEMW lvi = { 0 };
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT;// 
		lvi.iItem = ListView_GetItemCount(hList);
		lvi.pszText = const_cast<LPWSTR>(vecNodes[i].m_sName.data());//todo：这里用指针指向const内容
		// 图标：0=文件夹，1=文件
		lvi.iImage = vecNodes[i].m_bIsFolder ? 0 : 1;
		// 绑定自定义节点ID（关键：双击时识别是哪个节点）
		//这里保存u_Id是必要的，否则无法点进文件夹
		lvi.lParam = vecNodes[i].m_uId;
		lvi.iIndent = 1;
		//lvi.iSubItem = 1;
		const wchar_t* pszPath = NULL;
		pszPath = (lvi.iImage == 0) ? L"D:\\Tools" : L"D:\\Tools\\test.txt";//todo：待优化部分

		SHGetFileInfoW(
			pszPath,
			FILE_ATTRIBUTE_NORMAL,
			&sfi,
			sizeof(sfi),
			SHGFI_SYSICONINDEX | SHGFI_SMALLICON
		);
		lvi.iImage = sfi.iIcon;
		ListView_InsertItem(hList, &lvi);
	}
}

//void CListViewMgr::InsertBookMarkIntoFolder(HWND hList, std::optional<std::pair<std::string, std::string>> activeInfo, std::optional<CBookMarksNode> insertedFolder)
//{
//	CBookMarksNode newBookMark = m_vecNodes.back();
//	SHFILEINFOW sfi = { 0 };
//	// 插入ListView项
//	LVITEMW lvi = { 0 };
//	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT;// 
//	lvi.iItem = ListView_GetItemCount(hList);
//	lvi.pszText = newBookMark.m_sName.data();
//	// 图标：0=文件夹，1=文件
//	lvi.iImage = 1;
//	// 绑定自定义节点ID（关键：双击时识别是哪个节点）
//	//这里保存u_Id是必要的，否则无法点进文件夹
//	lvi.lParam = newBookMark.m_uId;
//	lvi.iIndent = 1;
//	//lvi.iSubItem = 1;
//	const wchar_t* pszPath = NULL;
//	pszPath = (lvi.iImage == 0) ? L"D:\\Tools" : L"D:\\Tools\\test.txt";//todo：待优化部分
//
//	SHGetFileInfoW(
//		pszPath,
//		FILE_ATTRIBUTE_NORMAL,
//		&sfi,
//		sizeof(sfi),
//		SHGFI_SYSICONINDEX | SHGFI_SMALLICON
//	);
//	lvi.iImage = sfi.iIcon;
//	ListView_InsertItem(hList, &lvi);
//}

void CListViewMgr::ListViewInsertColumn(HWND hWnd)
{
	//如果在CreateWindowW时使用了属性LVS_SMALLICON，后面的列就显示不出来了
	LVCOLUMNW lvc = { 0 };
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.pszText = const_cast<LPWSTR>(L"  名称");
	lvc.cx = 200;
	int nColIndex = ListView_InsertColumn(hWnd, 1, &lvc);
	if (nColIndex == -1) 
	{
		wchar_t szErr[256];
		int nErr = GetLastError();
		wsprintfW(szErr, L"插入列失败！错误码：%d", nErr);
	}
	else 
	{
		//MessageBoxW(hWnd, L"列插入成功！", L"成功", MB_OK);
	}
	lvc.pszText = const_cast<LPWSTR>(L"修改时间");
	lvc.cx = 50;

	ListView_InsertColumn(hWnd, 2, &lvc);
}

void CListViewMgr::InitSingleListView(HWND hListView)
{
	if (!hListView) return;

	//// ① 保存原始窗口过程（只保存一次）
	//if (g_OldListViewProc == NULL)
	//{
	//	g_OldListViewProc = (WNDPROC)GetWindowLongPtr(hListView, GWLP_WNDPROC);
	//}
	//// ② 子类化：让ListView用我们的窗口过程（拦截擦除）
	//SetWindowLongPtr(hListView, GWLP_WNDPROC, (LONG_PTR)ListViewSubProc);

	// ③ 可选：加ListView专属双缓冲（降低绘制压力，锦上添花）
	DWORD dwExStyle = ListView_GetExtendedListViewStyle(hListView);
	dwExStyle |= LVS_EX_DOUBLEBUFFER; // 仅核心双缓冲，无SDK兼容问题
	ListView_SetExtendedListViewStyle(hListView, dwExStyle);
}


void CListViewMgr::VisitSubListViewFolder(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, HWND hListView)
{
	/*NMITEMACTIVATE 是 Windows 通用控件中专门用于表示 “项被激活” 的通知结构
		—— 核心作用是：当用户通过点击、双击、按回车等方式 “激活” 控件中的某一项（比如列表视图、树视图、列表框的项）时，
		控件会通过 WM_NOTIFY 消息把这个结构传给父窗口，携带 “激活事件” 的详细信息。*/
	LPNMITEMACTIVATE pNMItem = (LPNMITEMACTIVATE)lParam;
	if (-1 == pNMItem->iItem)
	{
		return;
	}

	LVITEM lvItem = { 0 };
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = pNMItem->iItem;

	BOOL bRes = ListView_GetItem(hListView, &lvItem);
	if (FALSE == bRes)
	{
		return;
	}
	const std::vector<CBookMarksNode>& vecNodes = CBookMarksMgr::instance().GetAllBookMarksNodes();
	const unsigned& uNodeCount = CBookMarksMgr::instance().GetBookMarksCnt();
	//uint64_t uNodeCount = m_vecNodes.size();
	if (ID_BACK_TO_PARENT == lvItem.lParam)//如果点击的是“返回上一级”
	{
		std::optional<signed int> nParentId;
		for (unsigned int i = 0; i < uNodeCount; i++)
		{
			if (vecNodes[i].m_uNum == m_nLeftCurrentParent)
			{
				nParentId = vecNodes[i].m_nFatherNum;
				m_nLeftCurrentParent = vecNodes[i].m_nFatherNum;
				break;
			}
		}
		if (!nParentId.has_value())
		{
			return;
		}
		LoadVirtualFolders(hListView, nParentId.value());
	}
	else
	{
		std::optional<CBookMarksNode> node = FindVirtualFoldNode(lvItem.lParam);
		if (!node.has_value())
		{
			return;
		}

		if (node->m_bIsFolder)
		{
			// 进入文件夹：更新当前父节点，重新加载
			m_nLeftCurrentParent = node->m_uNum;
			LoadVirtualFolders(hListView, m_nLeftCurrentParent);
		}
		else if (!node->m_bIsFolder)
		{
			// 点击数据项：显示自定义数据
			WCHAR msg[512];
			wsprintfW(msg, L"自定义数据：\n名称：%s\n数据库ID：%d\n描述：%s",
				node->m_sName, node->m_uId, node->m_sDescription);
			MessageBoxW(hWnd, msg, L"自定义数据详情", MB_OK);
		}
	}
}

const unsigned int CListViewMgr::GetInitMainWndWidth()
{
	return m_nInitMainWndWidth;
}

const unsigned int CListViewMgr::GetInitSplitterWidth()
{
	return m_nInitSplitterWidth;
}

VOID CListViewMgr::RecoverRedrawListView()
{
	// 恢复 ListView 重绘，并一次性刷新（仅触发一次，无闪烁）
	if (m_hLeftListView)
	{
		SendMessage(m_hLeftListView, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(m_hLeftListView, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	if (m_hRightListView)
	{
		SendMessage(m_hRightListView, WM_SETREDRAW, TRUE, 0);
		RedrawWindow(m_hRightListView, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
}

HWND CListViewMgr::GetLeftListView()
{
	return m_hLeftListView;
}

std::optional<CBookMarksNode> CListViewMgr::FindIndexById(uint64_t uid)
{
	return CBookMarksMgr::instance().FindIndexById(uid);
}

BOOL CListViewMgr::SaveInsertedFolder(HWND hList, LPARAM lParam)
{
	std::lock_guard<std::mutex> lk(m_mtxToBeAddedNodes);
	NMHDR* pNMHDR = (NMHDR*)lParam;
	NMLISTVIEW* pNMLV = (NMLISTVIEW*)lParam;
	int nItem = pNMLV->iItem;
	if (nItem == -1)
	{
		return FALSE; // 无效
	}

	LVITEM lvi = { 0 };
	TCHAR szText[256] = { 0 };
	lvi.mask = LVIF_TEXT | LVIF_PARAM; // 同时读取文本+lParam
	lvi.iItem = nItem;
	lvi.iSubItem = 0;
	lvi.pszText = szText;
	lvi.cchTextMax = _countof(szText);

	ListView_GetItem(hList, &lvi);
	LPARAM userData = lvi.lParam;//此段代码可以正确获得ListViewItem名称

	//返回要插入的那个节点的信息
	//无论右键点击多少次ListViewItem，都只保存最新的一次
	//todo：这里要判断，如果右键点击的不是文件夹而是文件，另行处理
	m_InsertedFolder = FindIndexById(static_cast<uint64_t>(userData));
	if (!m_InsertedFolder.has_value())
	{
		return FALSE;
	}
	
	return TRUE;
}

std::optional<CBookMarksNode> CListViewMgr::GetInsertedFolder()
{
	std::lock_guard<std::mutex> lk(m_mtxToBeAddedNodes);
	return m_InsertedFolder;
}

void CListViewMgr::VisitListViewFolder(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NMHDR* pNMHDR = (NMHDR*)lParam;

	if (pNMHDR->code == NM_DBLCLK)
	{
		VisitSubListViewFolder(hWnd, msg, wParam, lParam, pNMHDR->hwndFrom);
	}
}

HWND CListViewMgr::GetRightListView()
{
	return m_hRightListView;
}

// 根据节点ID查找虚拟节点
//ItemNode* CListViewMgr::FindVirtualFoldNode(int node_id)
std::optional<CBookMarksNode> CListViewMgr::FindVirtualFoldNode(int node_id)
{
	const std::vector<CBookMarksNode>& vecNodes = CBookMarksMgr::instance().GetAllBookMarksNodes();
	const unsigned& uNodeCount = CBookMarksMgr::instance().GetBookMarksCnt();
	for (unsigned int i = 0; i < uNodeCount; i++)
	{
		if (vecNodes[i].m_uId == node_id)
		{
			return vecNodes[i];
		}
	}
	return std::nullopt;
}
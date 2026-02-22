#include "ListViewMgr.h"


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

//注意：const 全局变量默认是 internal linkage（内部链接）
extern const unsigned int g_nSplitterX = 400;           // 拆分条位置
extern const unsigned int g_nDefaultSubWindowWidth = 400;
extern const unsigned int g_nDefaultSplitterWidth = 10;
int g_nSplitterPos = 0;
// 当前层级：记录每个面板的当前父节点ID（模拟“当前目录”）
int g_left_current_parent = -1;
int g_right_current_parent = -1;
// 图标列表（文件夹/文件图标）

CListViewMgr::CListViewMgr():
	m_hImageList(NULL),
	m_bInit(TRUE),
	m_bDragging(FALSE)
{

}

CListViewMgr::~CListViewMgr()
{

}

BOOL CListViewMgr::InitDoubleListViewAndLoadData(HWND hWnd)
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
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS | WS_BORDER,
		0, 0, g_nDefaultSubWindowWidth, nListViewHeight, hWnd, NULL, GetModuleHandle(NULL), NULL);

	ListView_SetImageList(g_hLeftListView, m_hImageList, LVSIL_SMALL);

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
	ListView_SetImageList(g_hRightListView, m_hImageList, LVSIL_SMALL);

	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.pszText = const_cast<LPWSTR>(L"名称");
	lvc.cx = 100;
	ListView_InsertColumn(g_hRightListView, 0, &lvc);
	lvc.pszText = const_cast<LPWSTR>(L"修改时间");
	lvc.cx = 50;
	ListView_InsertColumn(g_hRightListView, 1, &lvc);
	LoadVirtualFolder(g_hRightListView, g_right_current_parent);

	return TRUE;
}

BOOL CListViewMgr::ShowDoubleListView(HWND hWnd)
{
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	int nSplitterWidth = g_nDefaultSplitterWidth;
	int nWidth = (rcClient.right - rcClient.left - g_nDefaultSplitterWidth) / 2;
	MoveWindow(g_hLeftListView, 0, 0, nWidth, rcClient.bottom, TRUE);
	MoveWindow(g_hSplitter, nWidth, 0, nSplitterWidth, rcClient.bottom, TRUE);
	MoveWindow(g_hRightListView, nWidth + nSplitterWidth, 0, nWidth, rcClient.bottom, TRUE);
	m_bInit = FALSE;

	return TRUE;
}

BOOL CListViewMgr::DragSplitterAndRefreshDoubleListView(HWND hWnd)
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

	return TRUE;
}

BOOL CListViewMgr::PressSplitter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT rcSplitter;
	GetWindowRect(g_hSplitter, &rcSplitter);
	POINT pt = { LOWORD(lParam), HIWORD(lParam) };
	ClientToScreen(hWnd, &pt); // 转换为屏幕坐标

	//判断鼠标是否在Splitter里
	if (PtInRect(&rcSplitter, pt))
	{
		m_bDragging = TRUE;
		// 设置鼠标捕获，确保拖动时能接收鼠标消息
		SetCapture(hWnd);
		// 改变鼠标光标为左右箭头
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
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
	g_nSplitterPos = pt.x;
	// 立即刷新布局（触发WM_SIZE）
	SendMessage(hWnd, WM_SIZE, 0, 0);
	// 保持鼠标光标样式
	SetCursor(LoadCursor(NULL, IDC_SIZEWE));
	return TRUE;
}

BOOL CListViewMgr::IsInitStatus()
{
	return m_bInit;
}

BOOL CListViewMgr::IsDraggingStatus()
{
	return m_bDragging;
}

VOID CListViewMgr::SetDraggingStatus(BOOL bStatus)
{
	m_bDragging = bStatus;
}

VOID CListViewMgr::DestoryImageList()
{
	ImageList_Destroy(m_hImageList);
}

BOOL CListViewMgr::InitImageList() 
{
	m_hImageList = ImageList_Create(
		16, 16, //创建一个 16×16 像素
		ILC_COLOR32 | ILC_MASK,//32 位真彩色带透明通道
		2, //初始可存 2 个图标
		0);//无自动扩容 的图像列表，用于存放你后续添加的文件夹 / 文件通用图标。
	if (m_hImageList == NULL) {
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
	int nFolderIconIndex = ImageList_AddIcon(m_hImageList, sfi.hIcon);
	DestroyIcon(sfi.hIcon);

	// 添加文件图标
	SHGetFileInfoW(L"*.txt", FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	nFolderIconIndex = ImageList_AddIcon(m_hImageList, sfi.hIcon);
	DestroyIcon(sfi.hIcon);

	return TRUE;
}


// 加载指定父节点下的所有虚拟节点到ListView
void CListViewMgr::LoadVirtualFolder(HWND hList, int parent_id)
{
	// 清空列表
	ListView_DeleteAllItems(hList);

	// 遍历自定义数据，加载对应节点
	for (unsigned int i = 0; i < g_nNodeCount; i++)
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
ItemNode* CListViewMgr::FindVirtualFoldNode(int node_id)
{
	for (unsigned int i = 0; i < g_nNodeCount; i++)
	{
		if (g_szTestNode[i].nID == node_id)
		{
			return &g_szTestNode[i];
		}
	}
	return NULL;
}
#pragma once
#include <Windows.h>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

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

typedef enum {
	PANEL_MODE_DOUBLE = 0,  // 双面板（默认）
	PANEL_MODE_QUAD = 1     // 四面板
} PanelMode;

class CListViewMgr
{
public:
	CListViewMgr();
	virtual ~CListViewMgr();
	/**************************************************************************
	* @brief 一上来创建两个窗口并加载初始数据
	* @param 
	* @return 
	*************************************************************************/
	BOOL InitDoubleListViewAndLoadData(HWND hWnd);

	/**************************************************************************
	* @brief 拖拽中间的Splitter的时候重绘两个ListView
	* @param
	* @return
	*************************************************************************/
	BOOL DragSplitterAndRefreshAllListView(HWND hWnd);

	/**************************************************************************
	* @brief 按住Splitter准备拖动
	* @param
	* @return
	*************************************************************************/
	BOOL PressSplitter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/**************************************************************************
	* @brief 拖动Splitter并通知WM_SIZE进行重绘
	* @param
	* @return
	*************************************************************************/
	BOOL DragSplitterAndSendMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	/**************************************************************************
	* @brief 绘制ListView时，判断是否是程序运行起来第一次初始化ListView
	* @param
	* @return
	*************************************************************************/
	BOOL IsInitStatus();

	/**************************************************************************
	* @brief 重绘ListView时，判断是不是正处于拖拽状态
	* @param
	* @return
	*************************************************************************/
	BOOL IsDraggingStatus();

	VOID SetDraggingStatus(BOOL bStatus);

	VOID Destory();

	BOOL TogglePanelMode(HWND hWnd);

	const unsigned int GetInitMainWndWidth();
	const unsigned int GetInitSplitterWidth();

	VOID RecoverRedrawListView();

private:
	CListViewMgr(const CListViewMgr& other);
	CListViewMgr& operator=(const CListViewMgr& other);

	/**************************************************************************
	* @brief 初始化图标列表（文件夹+文件图标）
	* @param
	* @return
	*************************************************************************/
	BOOL InitImageList();
	ItemNode* FindVirtualFoldNode(int node_id);
	void LoadVirtualFolder(HWND hList, int parent_id);

	void ListViewInsertColumn(HWND hWnd);

	//LRESULT CALLBACK ListViewSubProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	// 3. 初始化ListView的函数（给4个ListView都调用）
	void InitSingleListView(HWND hListView);
	
private:
	HIMAGELIST m_hImageList;
	BOOL m_bInit;
	BOOL m_bDragging;
	PanelMode m_PanelMode;
	HWND m_hVerticalSplitter; // 初始化时的那个纵向拆分条
	HWND m_hHorizontalSplitter; // 扩展成四个ListView时的那个横向拆分条
	HWND m_hLeftListView, m_hRightListView;   // 左右面板ListView
	HWND m_hTopLeftListView;
	HWND m_hTopRightListView;
	HWND m_hBottomLeftListView;
	HWND m_hBottomRightListView;

	unsigned int m_nCurrentSplitterX;	//拖动Splitter时的位置
	unsigned int m_nInitSplitterX;           // 拆分条位置
	unsigned int m_nInitListViewHeight;//初始展示的双ListView时的高度
	unsigned int m_nInitListViewWidth;//初始展示的双ListView时的宽
	unsigned int m_nInitMainWndWidth;
	unsigned int m_nLastSplitterX;
	const unsigned int m_nInitSplitterWidth;
};


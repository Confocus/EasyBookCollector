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
	* @brief 显示初始化的两个ListView
	* @param
	* @return
	*************************************************************************/
	BOOL ShowDoubleListView(HWND hWnd);

	/**************************************************************************
	* @brief 拖拽中间的Splitter的时候重绘两个ListView
	* @param
	* @return
	*************************************************************************/
	BOOL DragSplitterAndRefreshDoubleListView(HWND hWnd);

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

	VOID DestoryImageList();
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

private:
	HIMAGELIST m_hImageList;
	BOOL m_bInit;
	BOOL m_bDragging;
};


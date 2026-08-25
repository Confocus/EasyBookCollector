#pragma once
#include <Windows.h>
#include <commctrl.h>
#include "singleton.h"
#pragma comment(lib, "comctl32.lib")
#include "PipeMessageHandler.h"
#include "BookMarksNode.h"
#define MAX_NAME_LEN 256
#define ID_BACK_TO_PARENT	-1

// ===================== 自定义数据结构（模拟虚拟文件夹/数据项） =====================
// 虚拟节点：表示文件夹/数据项
typedef struct {
	unsigned int nID;				// 唯一ID
	BOOL bIsFolder;					// 是否是文件夹
	WCHAR szName[MAX_NAME_LEN];    // 显示名称
	unsigned int nParentId;      // 父节点ID（-1表示根节点）
	// 自定义数据：比如数据库ID、内容描述等
	int db_id;          // 模拟数据库ID
	WCHAR szDesc[512];    // 模拟自定义描述
} ItemNode;

typedef enum 
{
	DRAG_TYPE_STOP = 0,
	DRAG_TYPE_VIRTICAL = 1,
	DRAG_TYPE_HORIZONTAL = 2,
}DragType;

typedef enum {
	PANEL_MODE_DOUBLE = 0,  // 双面板（默认）
	PANEL_MODE_QUAD = 1     // 四面板
} PanelMode;

class CListViewMgr : public Singleton<CListViewMgr>
{
	//设置为友元可以调用private中的CListViewMgr的构造
	friend Singleton<CListViewMgr>;
public:

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

	BOOL ReleaseSplitter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
	BOOL IsSplitterDragged();

	/**************************************************************************
	* @brief 重绘ListView时，判断主窗口的边框是否正在被拖拽
	* @param
	* @return
	*************************************************************************/
	BOOL IsBorderDragged();

	VOID SetBorderDraggedStatus(BOOL);

	VOID SetDraggingStopStatus();

	VOID Destory();

	BOOL TogglePanelMode(HWND hWnd);

	const unsigned int GetInitMainWndWidth();
	const unsigned int GetInitSplitterWidth();

	VOID RecoverRedrawListView();
	/**************************************************************************
	* @brief 处理双击文件夹的操作
	* @param
	* @return
	*************************************************************************/
	void VisitListViewFolder(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND GetRightListView();
	HWND GetLeftListView();

	std::optional<CBookMarksNode> FindIndexById(uint64_t uid);

	/**************************************************************************
	* @brief 右键时将被右键点击的那个文件夹的信息传递给WM_COMMAND
	* @param
	* @return
	*************************************************************************/
	BOOL SaveInsertedFolder(HWND hList, LPARAM lParam);

	std::optional<CBookMarksNode> GetInsertedFolder();

	//void InsertBookMarkIntoFolder(HWND hList, std::optional<std::pair<std::string, std::string>> activeInfo, std::optional<CBookMarksNode> insertedFolder);

private:
	//Singleton<CListViewMgr>是友元可以调用private中的CListViewMgr的构造
	CListViewMgr();
	virtual ~CListViewMgr();

	CListViewMgr(const CListViewMgr& other);
	CListViewMgr& operator=(const CListViewMgr& other);

	VOID AdjustDoubleListView(HWND hWnd, unsigned int nMainWndWidth, unsigned int nCurrentVerticalSplitterX, unsigned int nListViewHeight, unsigned int nSplitterWidth);

	VOID AdjustQuadListView(HWND hWnd,
		unsigned int nMainWndWidth,
		unsigned int nCurrentVerticalSplitterX,
		unsigned int nListViewHeight,
		unsigned int nSplitterWidth,
		unsigned int nCurrentHorizontalSplitterY);

	/**************************************************************************
	* @brief 初始化图标列表（文件夹+文件图标）
	* @param
	* @return
	*************************************************************************/
	BOOL InitImageList();
	//ItemNode* FindVirtualFoldNode(int node_id);
	std::optional<CBookMarksNode> FindVirtualFoldNode(int node_id);
	void LoadVirtualFolders(HWND hList, int parent_id);

	void ListViewInsertColumn(HWND hWnd);

	//LRESULT CALLBACK ListViewSubProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//初始化ListView的函数（给4个ListView都调用）
	void InitSingleListView(HWND hListView);
	
	void VisitSubListViewFolder(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, HWND hListView);
private:
	HIMAGELIST m_hImageList;
	BOOL m_bInit;
	DragType m_eDraggingType;
	PanelMode m_PanelMode;
	HWND m_hVerticalSplitter; // 初始化时的那个纵向拆分条
	HWND m_hHorizontalSplitter; // 扩展成四个ListView时的那个横向拆分条
	HWND m_hLeftListView, m_hRightListView;   // 左右面板ListView
	HWND m_hTopLeftListView;
	HWND m_hTopRightListView;
	HWND m_hBottomLeftListView;
	HWND m_hBottomRightListView;
	HWND m_hToolTip;

	unsigned int m_nCurrentVerticalSplitterX;	//拖动Splitter时的位置
	unsigned int m_nCurrentHorizontalSplitterY;	//拖动Splitter时的位置
	unsigned int m_nInitSplitterX;           // 拆分条位置
	unsigned int m_nInitSplitterY;           // 拆分条位置
	unsigned int m_nInitListViewHeight;//初始展示的双ListView时的高度
	unsigned int m_nInitListViewWidth;//初始展示的双ListView时的宽
	unsigned int m_nInitMainWndWidth;
	unsigned int m_nLastSplitterX;
	const unsigned int m_nInitSplitterWidth;
	signed int m_nLeftCurrentParent;//ListView的Folder的父节点
	BOOL m_bIsBorderDragged;

	//右键添加某个书签时，将要被添加到的目录
	//但这里会出现如果手速过快，待添加的位置出现多个进行堆积，但还未真正处理完添加
	//这里要考虑添加和处理的匹配
	std::optional<CBookMarksNode> m_InsertedFolder;
	//std::vector<BookMarksNode> m_vecNodesToBeAdded;
	//ToBeAddedNodes会在另一个线程读取，不要在另一个线程读的时候修改这里，而且将来也可能改成队列
	//注意，std::mutex是不可拷贝的
	std::mutex m_mtxToBeAddedNodes;
	//显示数据时保存BookMarksNode列表在对象里，以免每次都更新
	// 但因为BookMarksMgr中会实时更新，数据，所以每次都必须重新获取数据
	//std::vector<CBookMarksNode> m_vecNodes;
};
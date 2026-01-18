#pragma once
#include <Windows.h>
#include <optional>

extern BOOL g_bIsMainWindowHide;
extern int g_nCurrentFrame;
extern int g_nSlideDistance;
extern const int g_nDefaultSlideFrames;
extern int g_nEdgeWidth;
#define ANIMATE_TIMER_ID 1001         // 动画定时器ID

const int EDGE_THRESHOLD = 5;

//todo:这个类需要写得更抽象一些，使其适用于所有的窗口的动作
//
class CMainWindowActions
{
public:
	CMainWindowActions(); 
	virtual ~CMainWindowActions();

	//判断窗口是否触碰到了边框
	std::optional<BOOL> IsMainWindowTouchScreenEdge(HWND hWnd);
	
	BOOL StartStimulateSlideHideWindowToRightEdge(HWND hWnd);
	VOID ProcessStimulateSlideHideWindowToRightEdge(HWND hWnd);
	VOID ShowHidedWindowFromRightSide(HWND hWnd);
	//判断鼠标是否在窗口右边
	std::optional<BOOL> IsMouseOnMainWindowRightEdge(HWND hWnd);
	BOOL GetObCursorOnRightEdge();
	VOID SetObCursorOnRightEdge(BOOL);
	std::optional<int> GetMainWindowTop(HWND hWnd);
	std::optional<int> GetMainWindowBottom(HWND hWnd);
	bool IsMouseReallyLeaveMainWnd(HWND hWnd);
private:
	BOOL m_bIsCursorOnMainWnd;
	BOOL m_bCursorOnRightEdge;
	//最开始时窗口所在的位置
	std::optional<int> m_nOriginalWindowLeft;
	std::optional<int> m_nOriginalWindowTop;
};
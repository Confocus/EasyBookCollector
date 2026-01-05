#include "functions.h"

const int ANIMATION_TIME = 800;
const int nFrameInterval = 10;

BOOL g_bIsMainWindowHide = FALSE;
int g_nCurrentFrame = 0;
int g_nSlideDistance = 0;
UINT_PTR g_uAnimTimerID = 1;         // 动画定时器ID
int g_nOriginalWindowLeft = 0;
const int g_nDefaultSlideFrames = 100;
int g_nEdgeWidth = 0;

std::optional<bool> IsMainWindowTouchScreenEdge(HWND hWnd)
{
	RECT rcWindow;
	if (!GetWindowRect(hWnd, &rcWindow))
	{
		return std::nullopt;
	}

	int nWindowRight = rcWindow.right;
	int nScreenRight = 0;
	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor != NULL)
	{
		MONITORINFO mi = { sizeof(MONITORINFO) };
		GetMonitorInfo(hMonitor, &mi);
		nScreenRight = mi.rcMonitor.right; // 物理屏幕右边缘
		//if (useWorkArea)
		//	nScreenRight = mi.rcWork.right; // 工作区右边缘（排除任务栏）
		//else

	}
	else
	{
		return std::nullopt;
	}

	return abs(nScreenRight - nWindowRight) < EDGE_THRESHOLD ? TRUE : FALSE;
}

BOOL StartStimulateSlideHideWindowToRightEdge(HWND hWnd)
{
	if (g_bIsMainWindowHide)
	{
		return FALSE;
	}

	//计算Windows的左边
	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);
	g_nOriginalWindowLeft = rcWindow.left;

	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor)
	{
		return FALSE;
	}
	MONITORINFO mi = { sizeof(MONITORINFO) };
	GetMonitorInfo(hMonitor, &mi);
	g_nSlideDistance = mi.rcWork.right - g_nOriginalWindowLeft;
	SetTimer(hWnd, g_uAnimTimerID, nFrameInterval, NULL);
	return TRUE;
}

//只能客户区移动，窗口的边框非客户区无法移动
//void SlideHideWindowToRightEdge(HWND hWnd)
//{
//	RECT rcWindow;
//	GetWindowRect(hWnd, &rcWindow);
//	int nScreenRight = 0;
//	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
//	if (hMonitor != NULL)
//	{
//		MONITORINFO mi = { sizeof(MONITORINFO) };
//		GetMonitorInfo(hMonitor, &mi);
//		nScreenRight = mi.rcWork.right;
//	}
//	else
//	{
//		
//	}
//	int screenWidth = GetSystemMetrics(SM_CXSCREEN);   // 屏幕宽度
//	int screenHeight = GetSystemMetrics(SM_CYSCREEN);  // 屏幕高度
//	int width = screenWidth / 5;   // 新宽度
//	int height = screenHeight / 2;  // 新高度
//	// 仅留5px窄条在屏幕内
//	//int nNewLeft = nScreenRight - ;
//	
//	BOOL bRes = AnimateWindow(hWnd, ANIMATION_TIME, AW_HOR_POSITIVE | AW_SLIDE | AW_HIDE);
//	INT64 nErr = GetLastError();
//	Sleep(1000);
//	SetWindowPos(hWnd, NULL, screenWidth - 100, rcWindow.top, 0, 0,
//		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
//	ShowWindow(hWnd, SW_SHOW);
//	//g_bIsHidden = true;
//}
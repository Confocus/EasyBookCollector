#include "CMainWindowActions.h"
const int ANIMATION_TIME = 800;
const int nFrameInterval = 1;

BOOL g_bIsMainWindowHide = FALSE;
int g_nCurrentFrame = 0;
int g_nSlideDistance = 0;
const int g_nDefaultSlideFrames = 100;
int g_nEdgeWidth = 0;


CMainWindowActions::CMainWindowActions() : 
	m_bIsCursorOnMainWnd(FALSE), m_bCursorOnRightEdge(FALSE)
{
}


CMainWindowActions::~CMainWindowActions()
{

}

std::optional<BOOL> CMainWindowActions::IsMainWindowTouchScreenEdge(HWND hWnd)
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

BOOL CMainWindowActions::StartStimulateSlideHideWindowToRightEdge(HWND hWnd)
{
	if (g_bIsMainWindowHide)
	{
		return FALSE;
	}

	//已经初始化过了后边再跑到这里也不赋值了
	if (!m_nOriginalWindowLeft.has_value() || !m_nOriginalWindowTop.has_value())
	{
		RECT rcWindow;
		GetWindowRect(hWnd, &rcWindow);//TODO：这里有没有出错的可能
		m_nOriginalWindowLeft = rcWindow.left;
		m_nOriginalWindowTop = rcWindow.top;
	}
	
	HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor)
	{
		return FALSE;
	}

	MONITORINFO mi = { sizeof(MONITORINFO) };
	GetMonitorInfo(hMonitor, &mi);
	g_nSlideDistance = mi.rcWork.right - m_nOriginalWindowLeft.value();
	SetTimer(hWnd, ANIMATE_TIMER_ID, nFrameInterval, NULL);
	return TRUE;
}

VOID CMainWindowActions::ProcessStimulateSlideHideWindowToRightEdge(HWND hWnd)
{
	g_nCurrentFrame++;

	double dProcess = (static_cast<double>(g_nCurrentFrame) / static_cast<double>(g_nDefaultSlideFrames));//待滑动的百分比的进度
	double dSlideWidth = dProcess * static_cast<double>(g_nSlideDistance - g_nEdgeWidth);//相较于原始左边起点每次待滑动的距离
	int nCurrentWindowLeft = m_nOriginalWindowLeft.value() + dSlideWidth;

	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);
	SetWindowPos(hWnd, NULL,
		nCurrentWindowLeft, rcWindow.top,
		0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	if (g_nCurrentFrame >= g_nDefaultSlideFrames)
	{
		KillTimer(hWnd, ANIMATE_TIMER_ID);
		g_bIsMainWindowHide = TRUE;//此时已经隐藏好
		g_nCurrentFrame = 0;//恢复等于0，以留着下次计算使用
	}
}

VOID CMainWindowActions::ShowHidedWindowFromRightSide(HWND hWnd)
{
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);   // 屏幕宽度
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);  // 屏幕高度
	int width = screenWidth / 5;   // 新宽度
	int height = screenHeight / 2;  // 新高度

	SetWindowPos(hWnd, NULL,
		m_nOriginalWindowLeft.value(), m_nOriginalWindowTop.value(),
		width, height,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	g_bIsMainWindowHide = FALSE;
}

std::optional<BOOL> CMainWindowActions::IsMouseOnMainWindowRightEdge(HWND hWnd)
{
	POINT ptMouse;
	GetCursorPos(&ptMouse); // 拿到鼠标的x/y（屏幕坐标系）
	//TODO：这里拿到的是负值，修改这里的坐标系
	RECT rcMainWnd;
	if (!GetWindowRect(hWnd, &rcMainWnd))
	{
		return std::nullopt; // 窗口无效，直接返回false
	}
	/*m_bCursorOnRightEdge = (ptMouse.x > rcMainWnd.right) || abs(ptMouse.x - rcMainWnd.right) < 10;
	return m_bCursorOnRightEdge;*/
	return (ptMouse.x > rcMainWnd.right) || abs(ptMouse.x - rcMainWnd.right) < 10;
}

BOOL CMainWindowActions::GetObCursorOnRightEdge()
{
	return m_bCursorOnRightEdge;
}

VOID CMainWindowActions::SetObCursorOnRightEdge(BOOL bValue)
{
	m_bCursorOnRightEdge = bValue;
}

std::optional<int> CMainWindowActions::GetMainWindowTop(HWND hWnd)
{
	RECT rcWindow;
	if (!GetWindowRect(hWnd, &rcWindow))
	{
		return std::nullopt;
	}

	return rcWindow.top;
}

std::optional<int> CMainWindowActions::GetMainWindowBottom(HWND hWnd)
{
	RECT rcWindow;
	if (!GetWindowRect(hWnd, &rcWindow))
	{
		return std::nullopt;
	}

	return rcWindow.bottom;
}

bool CMainWindowActions::IsMouseReallyLeaveMainWnd(HWND hWnd)
{
	// 1. 获取主窗口的屏幕矩形（包含边框、标题栏、所有子控件）
	RECT rcMainWnd;
	GetWindowRect(hWnd, &rcMainWnd);

	// 2. 获取当前鼠标的屏幕坐标
	POINT ptCursor;
	GetCursorPos(&ptCursor);

	// 3. 判定：鼠标是否在主窗口矩形外 → 才是真的离开
	bool bLeave = !PtInRect(&rcMainWnd, ptCursor);

	return bLeave;
}
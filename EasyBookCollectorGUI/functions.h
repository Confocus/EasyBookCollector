#pragma once
#include <Windows.h>
#include <optional>

extern BOOL g_bIsMainWindowHide;
extern int g_nCurrentFrame;
extern int g_nSlideDistance;
extern UINT_PTR g_uAnimTimerID;         // 动画定时器ID
extern const int g_nDefaultSlideFrames;
extern int g_nOriginalWindowLeft;
extern int g_nEdgeWidth;

const int EDGE_THRESHOLD = 5;
//未支持分屏多屏幕的状态
std::optional<bool> IsMainWindowTouchScreenEdge(HWND hWnd);
//void SlideHideWindowToRightEdge(HWND hWnd);
BOOL StartStimulateSlideHideWindowToRightEdge(HWND hWnd);
VOID ProcessStimulateSlideHideWindowToRightEdge(HWND hWnd);
VOID ShowHidedWindowFromRightSide(HWND hWnd);
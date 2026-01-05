#pragma once
#include <Windows.h>
#include <optional>

const int EDGE_THRESHOLD = 5;
//未支持分屏多屏幕的状态
std::optional<bool> IsMainWindowTouchScreenEdge(HWND hWnd);
void SlideHideWindowToRightEdge(HWND hWnd);
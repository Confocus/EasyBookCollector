#include "ListBoxWndManager.h"
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <tchar.h>
#include <windows.h> 
#include <commctrl.h>       // 补充：ListBox的扩展样式/通知码 都在这里！！
#pragma comment(lib, "comctl32.lib") // 配套库文件，ListBox自绘/高级功能必须加
#include "Resource.h"
extern HINSTANCE g_hInstance;
extern CListBoxWndManager g_ListBoxWndMgr;

const int g_nListSpace = 10;
const int g_nListWidth = 180;
const int g_nListHeight = 300;

LRESULT CALLBACK ListBoxWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
		if (pDIS->CtlID == ID_SUB_LISTBOX_START && pDIS->CtlType == ODT_LISTBOX && pDIS->itemID != LB_ERR)
		{
			HDC hdc = pDIS->hDC;
			RECT rc = pDIS->rcItem;
			int nItem = pDIS->itemID;

			rc.top += 3;
			rc.bottom -= 3;
			rc.left += 6;
			rc.right -= 6;

			HBRUSH hBrush;
			if (pDIS->itemState & ODS_SELECTED)
			{
				hBrush = CreateSolidBrush(RGB(202, 220, 250)); // Win11淡蓝色高亮
			}
			else
			{
				hBrush = CreateSolidBrush(RGB(0, 255, 255)); // 纯白色背景
			}
			FillRect(hdc, &rc, hBrush);
			DeleteObject(hBrush);

			int nLen =static_cast<int>(SendMessage(pDIS->hwndItem, LB_GETTEXTLEN, nItem, 0));
			std::wstring wText;
			wText.resize(nLen + 1);
			SendMessage(pDIS->hwndItem, LB_GETTEXT, nItem, (LPARAM)wText.data());
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, RGB(20, 20, 20));
			RECT rcText = rc;
			//rcText.left += 35;
			DrawText(hdc, wText.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

			return TRUE; // 告诉系统绘制完成
		}
		break;
	}
	// ✅ 处理ListBox2的选中事件 LBN_SELCHANGE，和你之前的逻辑一致
	case WM_COMMAND:
	{
		if (HIWORD(wParam) == LBN_SELCHANGE)//点击主窗口的Listbox选项会走到这里
		{
			g_ListBoxWndMgr.ShowOrHideNode(hWnd, message, wParam, lParam);
		}
		break;
	}
	// ✅ 窗口2关闭时，销毁句柄，防止泄漏
	case WM_DESTROY:
	{
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

CListBoxWndManager::CListBoxWndManager():
	mIsListBoxWindowRegistered(FALSE),
	m_hWindow(NULL),
	m_hListBox(NULL),
	m_spTree(std::make_shared<CListBoxWindowTree>())
{

}

CListBoxWndManager::~CListBoxWndManager()
{

}

BOOL CListBoxWndManager::ShowOrHideNode(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
	do 
	{
		//hSenderList和hWindows是一对一的关系
		HWND hSenderList = (HWND)lParam;
		unsigned int selectedIndex = static_cast<unsigned int>(SendMessage(hSenderList, LB_GETCURSEL, 0, 0));
		if (selectedIndex == LB_ERR)
		{
			break;
		}

		unsigned int itemId = (unsigned int)SendMessage(hSenderList, LB_GETITEMDATA, selectedIndex, 0);

		std::optional<std::shared_ptr<CListBoxWindowNode>> optSpParentNode = GetNodePointerByHandle(hSenderList);
		std::shared_ptr<CListBoxWindowNode> spSonNode = (*optSpParentNode)->GetSonNode(itemId);

		//已经创建过了，且处于显示状态，则直接隐藏
		if (nullptr != spSonNode && spSonNode->GetIsShowed())
		{
			ShowWindow(spSonNode->GetCurrentHWND(), SW_HIDE);
			spSonNode->SetIsShowed(FALSE);
			bRet = TRUE;//todo:可以设置不同的状态，表达不同的返回原因
			break;
		}

		//已经创建过了，且处于未显示状态，则直接显示
		if (nullptr != spSonNode && !spSonNode->GetIsShowed())
		{
			ShowWindow(spSonNode->GetCurrentHWND(), SW_SHOW);
			spSonNode->SetIsShowed(TRUE);
			bRet = TRUE;
			break;
		}
		//拿到父节点所在的层级
		std::optional<int> nLevel = GetLevelBySenderHandle(hSenderList);
		if (!nLevel.has_value())
		{
			break;
		}

		RECT rcListBox;
		GetWindowRect(hSenderList, &rcListBox);

		//先创建节点
		//todo:每个子节点创建的位置可能要改一下
		std::optional<std::shared_ptr<CListBoxWindowNode>> spNewNode = CreateListBoxWindowNodeAndShow(hSenderList, rcListBox.left - 3 * g_nListSpace - g_nListWidth, rcListBox.top, g_nListWidth, g_nListHeight);
		if (!spNewNode.has_value())
		{
			break;
		}
		(*spNewNode)->SetIsShowed(TRUE);

		//插入节点到树结构中
		//todo:甚至我觉得不必建立树结构，而是简历一个set结构，然后set中的每个Node之间建立好关系链接即可
		InsertNodeToTree(nLevel.value() + 1, spNewNode.value());

		//建立父子节点之间的联系
		BindParentAndSonNode(itemId, *optSpParentNode, *spNewNode);
		bRet = TRUE;
		break;
	} while (FALSE);

	return bRet;
}

BOOL CListBoxWndManager::BuildListBoxWindowTree(HWND hWnd, HWND hListbox)
{
	std::shared_ptr<CListBoxWindowNode> spRoot = std::make_shared<CListBoxWindowNode>(hWnd, hListbox, TRUE);
	return m_spTree->InsertListBoxWindowNode(0, spRoot);
	//return m_spTree->BuildListBoxWindowTree(spRoot);
}

std::optional<std::shared_ptr<CListBoxWindowNode>> CListBoxWndManager::CreateListBoxWindowNodeAndShow(HWND hSender, int x, int y, int width, int height)//HINSTANCE hInst, 
{
	m_hWindow = CreateWindowEx(
		WS_EX_TOOLWINDOW,          // 常用于浮动工具窗口
		LISTBOX_WINDOW_CLASS_NAME,
		L"test",
		WS_POPUP | WS_VISIBLE,
		x, y, width, height,//rcListBox.left - 3 * g_nListSpace - g_nListWidth, rcListBox.top, g_nListWidth, g_nListHeight
		nullptr,                   
		nullptr,
		g_hInstance,
		nullptr
	);
	if (NULL == m_hWindow)
	{
		DWORD err = GetLastError();
		//todo:载入日志模块后输出错误
		return std::nullopt;
	}

	m_hListBox = CreateWindowEx(
		0,
		WC_LISTBOX,
		TEXT(""),
		WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS,
		0, 0, 50, 200, // 铺满整个窗口2
		m_hWindow, (HMENU)ID_SUB_LISTBOX_START, g_hInstance, NULL
	);
	if (NULL == m_hListBox)
	{
		DWORD err = GetLastError();
		//todo:载入日志模块后输出错误
		return std::nullopt;
	}

	SendMessage(m_hListBox, LB_SETITEMHEIGHT, 0, 28);//设置ListBox2的行高（自绘必加，唯一的规则）
	SendMessage(m_hListBox, LB_RESETCONTENT, 0, 0); // 清空数据

	static std::vector<std::wstring> vItem = { TEXT("text1"), TEXT("text2"), TEXT("text3"), TEXT("text4") };//
	std::for_each(vItem.begin(), vItem.end(), [=](const std::wstring& item) {
		SendMessage(m_hListBox, LB_ADDSTRING, 0, (LPARAM)item.c_str());
		});
	ShowWindow(m_hWindow, SW_SHOW);
	UpdateWindow(m_hWindow);

	return std::make_shared<CListBoxWindowNode>(m_hWindow, m_hListBox, TRUE);
}

BOOL CListBoxWndManager::RegisterListBoxWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEXW);

	BOOL bIsRegistered = GetClassInfoExW(hInstance, LISTBOX_WINDOW_CLASS_NAME, &wc);
	if (bIsRegistered)
	{
		return TRUE;
	}

	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = ListBoxWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EASYBOOKCOLLECTORGUI));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = LISTBOX_WINDOW_CLASS_NAME;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	ATOM clsAtom = RegisterClassExW(&wcex);
	if (clsAtom != 0)
	{
		return TRUE;
	}
	else
	{
		DWORD dwErr = GetLastError();
		return FALSE;
	}
}

BOOL CListBoxWndManager::InsertNodeToTree(unsigned int nIndex, std::shared_ptr<CListBoxWindowNode> spNode)
{
	if (NULL == spNode)
	{
		return FALSE;
	}
	return m_spTree->InsertListBoxWindowNode(nIndex, spNode);
}

BOOL CListBoxWndManager::BindParentAndSonNode(unsigned int nMapIndex, std::shared_ptr<CListBoxWindowNode> spParentNode, std::shared_ptr<CListBoxWindowNode> spSonNode)
{
	//告知父节点自己的某个ListboxItem有了一个子节点
	spParentNode->AddSonNode(nMapIndex, spSonNode);
	//给子节点配置父节点
	spSonNode->SetParentNode(spParentNode);
	//设置子节点对应的ListboxItem的ID
	spSonNode->SetParentListboxIndex(nMapIndex);
	return TRUE;
}

std::optional<unsigned int> CListBoxWndManager::GetLevelBySenderHandle(HWND hSender) 
{
	
	return m_spTree->GetListBoxLevelBySenderHandle(hSender); // 没找到
}

std::optional<std::shared_ptr<CListBoxWindowNode>> CListBoxWndManager::GetNodePointerByHandle(HWND hWnd)
{
	return m_spTree->GetNodePointerByHandle(hWnd);
}

BOOL CListBoxWndManager::IsSubListBoxShowed(HWND hSender)
{
	//todo:创建hSender和Node的对应关系表，便于查找
	//或者把hSender的值存到Node中，然后遍历树去查找
	//在树中保存每个Listbox是否弹出了SubListbox
	return TRUE;
}
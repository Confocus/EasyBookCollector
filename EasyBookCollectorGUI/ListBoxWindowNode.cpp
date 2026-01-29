#include "ListBoxWindowNode.h"
#include <algorithm>

CListBoxWindowNode::CListBoxWindowNode():
	m_hWindow(NULL),
	m_hListBox(NULL),
	m_IsShowed(FALSE),
	m_nListboxIndex(0)
{

}

CListBoxWindowNode::CListBoxWindowNode(HWND hWindow, HWND hListBox, BOOL bIsShowed /*= FALSE*/):
	CListBoxWindowNode()
{
	m_hWindow = hWindow;
	m_hListBox = hListBox;
	m_IsShowed = bIsShowed;
}

CListBoxWindowNode::~CListBoxWindowNode()
{

}

VOID CListBoxWindowNode::SetFromCursel(unsigned int nCursel)
{
	m_FromCursel = nCursel;
}

HWND CListBoxWindowNode::GetListBoxHwnd()
{
	return m_hListBox;
}

VOID CListBoxWindowNode::SetIsShowed(BOOL bShowed)
{
	m_IsShowed = bShowed;
}

BOOL CListBoxWindowNode::GetIsShowed()
{
	return m_IsShowed;
}

//VOID CListBoxWindowNode::SetIsSubWndShowed(BOOL bShowed)
//{
//	m_IsSubWndShowed = bShowed;
//}
//
//BOOL CListBoxWindowNode::GetIsSubWndShowed()
//{
//	return m_IsSubWndShowed;
//}

VOID CListBoxWindowNode::SetParentNode(std::shared_ptr<CListBoxWindowNode> spParentNode)
{

}

std::shared_ptr<CListBoxWindowNode> CListBoxWindowNode::GetParentNode()
{
	return NULL;
}

VOID CListBoxWindowNode::AddSonNode(unsigned int nMapIndex,std::shared_ptr<CListBoxWindowNode> spSonNode)
{
	m_mapSonNodes[nMapIndex] = spSonNode;
}

std::shared_ptr<CListBoxWindowNode>& CListBoxWindowNode::GetSonNode(unsigned int nMapIndex)
{
	return m_mapSonNodes[nMapIndex];
}

VOID CListBoxWindowNode::SetParentListboxIndex(unsigned int nMapIndex)
{
	m_nListboxIndex = nMapIndex;
}

HWND CListBoxWindowNode::GetCurrentHWND()
{
	return m_hWindow;
}
#include "ListBoxWindowNode.h"
#include <algorithm>

CListBoxWindowNode::CListBoxWindowNode():
	m_hWindow(NULL),
	m_hListBox(NULL),
	m_IsShowed(FALSE),
	m_Index(0)
{

}

CListBoxWindowNode::CListBoxWindowNode(HWND hWindow, HWND hListBox, BOOL bIsShowed /*= FALSE*/)
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
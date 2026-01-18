#include "ListBoxWindow.h"

CListBoxWindowUnit::CListBoxWindowUnit():
	m_hWindow(NULL),
	m_hListBox(NULL),
	m_IsShowed(FALSE),
	m_Index(0)
{

}

CListBoxWindowUnit::CListBoxWindowUnit(HWND hWindow, HWND hListBox, BOOL bIsShowed /*= FALSE*/)
{
	m_hWindow = hWindow;
	m_hListBox = hListBox;
	m_IsShowed = bIsShowed;
}

CListBoxWindowUnit::~CListBoxWindowUnit()
{

}

VOID CListBoxWindowUnit::SetFromCursel(unsigned int nCursel)
{
	m_FromCursel = nCursel;
}



CListBoxWindowTree::CListBoxWindowTree()
{
	m_vecTree.resize(5);
}

CListBoxWindowTree::~CListBoxWindowTree()
{

}

VOID CListBoxWindowTree::BuildListBoxWindowTree(std::shared_ptr<CListBoxWindowUnit> spUnit)
{
	m_spRoot = spUnit;
	m_vecTree.at(0).push_back(spUnit);
}

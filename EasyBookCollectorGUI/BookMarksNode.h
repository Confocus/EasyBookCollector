#pragma once
#include <windows.h>
#include <iostream>

class CBookMarksNode
{
public:
	CBookMarksNode() :
		m_bIsFolder(TRUE),
		m_uNum(0),
		m_nFatherNum(-1),
		m_uId(0)
	{

	}
	~CBookMarksNode()
	{

	}

	uint64_t GetNum();

	BOOL IsNodeFolder();
public:
	BOOL m_bIsFolder;
	uint64_t m_uNum;
	int64_t m_nFatherNum;
	uint64_t m_uId; 
	std::wstring m_sDescription;
	std::wstring m_sName;
};


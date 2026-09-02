#pragma once
#include <windows.h>
#include <iostream>

class CBookMarksNode
{
public:
	CBookMarksNode() :
		m_bIsFolder(TRUE),
		m_nFatherId(-1),
		m_uId(0)
	{

	}
	~CBookMarksNode()
	{

	}

	//uint64_t GetNum();
	int64_t GetId() const;
	BOOL IsNodeFolder();
public:
	BOOL m_bIsFolder;
	//uint64_t m_uNum;
	int64_t m_nFatherId;
	uint64_t m_uId; 
	std::wstring m_sDescription;
	std::wstring m_sName;
	std::wstring m_wsParentId;
	std::wstring m_wsBookmarkId;
};


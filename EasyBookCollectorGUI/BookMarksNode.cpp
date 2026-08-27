#include "BookMarksNode.h"

uint64_t CBookMarksNode::GetNum()
{
	return m_uNum;
}

BOOL CBookMarksNode::IsNodeFolder()
{
	return m_bIsFolder;
}

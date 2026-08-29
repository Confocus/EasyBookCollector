#include "BookMarksNode.h"

int64_t CBookMarksNode::GetId() const
{
	return m_uId;
}

//uint64_t CBookMarksNode::GetNum()
//{
//	return m_uNum;
//}

BOOL CBookMarksNode::IsNodeFolder()
{
	return m_bIsFolder;
}

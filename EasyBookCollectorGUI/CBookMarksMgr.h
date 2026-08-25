#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <tchar.h>
#include <thread>
#include "MainWindowActions.h"
#include <shellapi.h>
#include <array>
#include <queue>
#include <mutex>
#include <map>
#include "BookMarksNode.h"
#include "singleton.h"
#include <shared_mutex>


template<typename T>
class CThreadSafeVector
{
public:
	VOID Append(T);
	unsigned GetSize();
	std::vector<T> GetData() const;

	template<typename F>
	std::optional<T> FindIf(F&& func);
private:
	mutable std::shared_mutex m_rwLock;
	std::vector<T> m_vecData;

};

template<typename T>
std::vector<T> CThreadSafeVector<T>::GetData() const
{
	std::shared_lock<std::shared_mutex> lock(m_rwLock);
	return m_vecData;
}

template<typename T>
template<typename F>
std::optional<T> CThreadSafeVector<T>::FindIf(F&& func)
{
	std::shared_lock<std::shared_mutex> lock(m_rwLock);
	auto it = find_if(m_vecData.begin(), m_vecData.end(), func);

	if (it == m_vecData.end())
		return std::nullopt; // 没找到

	return *it;
}

template<typename T>
unsigned CThreadSafeVector<T>::GetSize()
{
	std::shared_lock<std::shared_mutex> lock(m_rwLock);
	return m_vecData.size();
}

template<typename T>
VOID CThreadSafeVector<T>::Append(T element)
{
	std::unique_lock<std::shared_mutex> lock(m_rwLock);
	m_vecData.push_back(element);
}

/**
 * @brief 对书签数据进行操作的组建
 * @details 比如增删改查之类
 */
class CBookMarksMgr : public Singleton<CBookMarksMgr>
{
	friend Singleton<CBookMarksMgr>;
public:
	VOID InsertFolder(const std::wstring);
	VOID InsertBookInfoUnderFolder(const std::wstring, const std::wstring, int64_t nFatherNum);
	std::vector<CBookMarksNode> GetAllBookMarksNodes() const;
	uint64_t GetBookMarksCnt() const;
	std::optional<CBookMarksNode> FindIndexById(uint64_t uid);
	VOID InsertNewAddedNode();
	CBookMarksNode& GetCurrentNode();

private:
	CBookMarksMgr();
	~CBookMarksMgr();
	CBookMarksMgr(const CBookMarksMgr & other) = delete;
	CBookMarksMgr& operator=(const CBookMarksMgr& other) = delete;
	CBookMarksMgr(CBookMarksMgr&& other) = delete;
	CBookMarksMgr& operator=(CBookMarksMgr&& other) = delete;

private:
	uint64_t m_uBookMarkNodeId = 0;
	CBookMarksNode m_uCurrentNode;
	mutable CThreadSafeVector<CBookMarksNode> m_vecThreadSafeNodes; //每一个文件夹或文件都被当做一个Node保存到了这个数组里
	std::vector<CBookMarksNode> m_vecLastNodes;
	std::vector<std::wstring> m_vecLastFolders;//保存上一次操作的文件夹路径序列，便于判断下一次从哪开始插入
	int64_t m_nLastFatherNum;
	//std::wstring sFolderName;//文件夹的名字、自己的名字
	//std::vector<std::wstring> vecBooks;
	//std::vector<std::shared_ptr<BookMarksTree*>> vecFolders;
};

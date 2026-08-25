
template<typename T>
class CThreadSafeVector
{
public:
	VOID Append(T);
	unsigned GetSize();
	const std::vector<T>& GetData() const;

	template<typename F>
	std::optional<T> FindIf(F&& func);
private:
	mutable std::shared_mutex m_rwLock;
	std::vector<T> m_vecData;

};

template<typename T>
const std::vector<T>& CThreadSafeVector<T>::GetData() const
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
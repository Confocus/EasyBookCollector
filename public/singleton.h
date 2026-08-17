#ifndef SINGLETON_H
#define SINGLETON_H

template<typename T>
class Singleton
{
public:
	// 获取唯一实例
	//这里是static所以不会是相互的循环定义
	static T& instance()
	{
		static T obj; // C++11标准保证静态局部变量初始化线程安全
		return obj;
	}

	// 删除拷贝、移动。为了让继承它的类也同时就具有了不可复制或赋值的功能
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;

protected:
	Singleton() = default;
	~Singleton() = default;
};

/**************************************************************************
继承方式
class Logger : public Singleton<Logger>
{
	friend class Singleton<Logger>;

private:
	Logger() = default;
};
*************************************************************************/

/**************************************************************************
调用方式：
Logger::instance();
要把类本身的构造函数设置为private，免得外部可以调用：
Logger logger;
*************************************************************************/


#endif
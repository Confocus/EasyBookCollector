#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>   // Windows 下需要

using namespace std;

// 读取浏览器发来的 JSON 消息
string readMessage()
{
	uint32_t length = 0;

	// 读 4 字节长度
	if (!cin.read(reinterpret_cast<char*>(&length), 4))
		return "";

	vector<char> buffer(length);

	// 读 JSON 内容
	cin.read(buffer.data(), length);

	return string(buffer.begin(), buffer.end());
}

// 向浏览器发送 JSON 消息
void sendMessage(const string& msg)
{
	uint32_t length = msg.size();

	// 写入长度（4 字节 Little Endian）
	cout.write(reinterpret_cast<char*>(&length), 4);

	// 写入 JSON 数据
	cout.write(msg.c_str(), length);

	cout.flush();
}

int main()
{
	ios_base::sync_with_stdio(false);

	std::ofstream log("..\\native_host.log", std::ios::app);
	log << "start success: " << std::endl;

	while (true)
	{
		string msg = readMessage();
		if (msg.empty())
			break;

		cerr << "Received from browser: " << msg << endl;

		// 这里可以解析 JSON（如 nlohmann/json）
		// 简单演示直接返回消息

		string reply =
			R"({"status":"ok","reply":"Hello from C++ Native Host"})";

		sendMessage(reply);
	}

	return 0;
}
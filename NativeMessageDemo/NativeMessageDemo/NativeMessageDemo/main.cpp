//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <string>
//#include <windows.h>   // Windows 下需要
//
//using namespace std;
//
//// 读取浏览器发来的 JSON 消息
//string readMessage()
//{
//	uint32_t length = 0;
//	std::ofstream log("..\\native_host.log", std::ios::app);
//
//	// 读 4 字节长度
//	if (!cin.read(reinterpret_cast<char*>(&length), 4))
//		return "";
//
//	log << length << std::endl;
//	vector<char> buffer(length);
//
//	// 读 JSON 内容
//	cin.read(buffer.data(), length);
//	log << string(buffer.begin(), buffer.end()) << std::endl;
//
//	return string(buffer.begin(), buffer.end());
//}
//
//// 向浏览器发送 JSON 消息
//void sendMessage(const string& msg)
//{
//	uint32_t length = msg.size();
//
//	// 写入长度（4 字节 Little Endian）
//	cout.write(reinterpret_cast<char*>(&length), 4);
//
//	// 写入 JSON 数据
//	cout.write(msg.c_str(), length);
//
//	cout.flush();
//}
//
//int main()
//{
//	ios_base::sync_with_stdio(false);
//
//	std::ofstream log("..\\native_host.log", std::ios::app);
//	log << "start success: " << std::endl;
//
//	while (true)
//	{
//		string msg = readMessage();
//		if (msg.empty())
//			break;
//
//		cerr << "Received from browser: " << msg << endl;
//
//		// 这里可以解析 JSON（如 nlohmann/json）
//		// 简单演示直接返回消息
//
//		string reply =
//			R"({"status":"ok","reply":"Hello from C++ Native Host"})";
//
//		sendMessage(reply);
//	}
//
//	return 0;
//}


#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <D:/AllProjects/public/json-develop/single_include/nlohmann/json.hpp>  // 依赖：nlohmann/json 库（处理 JSON 解析/生成）
#include "../../../../public/PipeMgr.h"
// 引入 JSON 库（nlohmann/json，可从 https://github.com/nlohmann/json 下载，直接包含头文件即可）
using json = nlohmann::json;

// 函数：从 stdin 读取 Native Messaging 消息
json readNativeMessage() {
	// 1. 先读取 4 字节无符号整数（小端序），获取 JSON 字符串长度
	uint32_t messageLength = 0;
	std::cin.read(reinterpret_cast<char*>(&messageLength), sizeof(messageLength));

	// 2. 读取对应长度的 JSON 字符串字节数据
	std::vector<char> messageBuffer(messageLength);
	std::cin.read(messageBuffer.data(), messageLength);

	// 3. 解析为 JSON 对象并返回
	std::string jsonString(messageBuffer.begin(), messageBuffer.end());
	return json::parse(jsonString);
}

// 函数：向 stdout 写入 Native Messaging 响应
void sendNativeMessage(const json& response) {
	// 1. 生成 JSON 字符串（UTF-8 编码）
	std::string jsonString = response.dump();

	// 2. 获取 JSON 字符串的字节长度，封装为 4 字节无符号整数（小端序）
	uint32_t messageLength = static_cast<uint32_t>(jsonString.size());

	// 3. 先写入 4 字节长度（必须先写，这是协议要求）
	std::cout.write(reinterpret_cast<const char*>(&messageLength), sizeof(messageLength));

	// 4. 再写入 JSON 字符串字节数据
	std::cout.write(jsonString.data(), messageLength);

	// 5. 刷新 stdout，确保数据立即发送（关键，避免缓存）
	std::cout.flush();
}
//todo:加上Mutex防止每次双击都调起一个exe
// todo:每次退出时断开std::out时会崩溃
// 主函数：处理扩展消息，返回响应
int main2() {
	try {
		// 循环读取扩展消息（Native Messaging 宿主程序通常持续运行，处理多条消息）
		while (true) {
			// 1. 读取扩展发送的消息
			json request = readNativeMessage();
			try {
				std::string dumpStr = request.dump(); // 单独提取dump结果，便于排查
				//这里之前用了js和exe客户端建立了通信连接，是不是当时是基于标准输入输出的通信连接？那么我是不是就不能再调用
				//std::cout << "【C++ exe】接收到扩展数据：" << dumpStr << std::endl;
			}
			catch (const std::exception& e) {
				// 捕获标准异常，打印具体错误信息
				std::cerr << "【崩溃原因】dump()抛出异常：" << e.what() << std::endl;
			}
			catch (...) {
				// 捕获所有非标准异常
				std::cerr << "【崩溃原因】dump()抛出未知异常！" << std::endl;
			}
			// 2. 处理数据（你的业务逻辑，示例：提取字段，写入日志文件）
			/*std::string targetTag = request.value("targetTag", "未知元素");
			std::string pageUrl = request.value("pageUrl", "未知URL");
			std::string logContent = "[" + std::string(__TIME__) + "]元素标签：" + targetTag + "，网页URL：" + pageUrl + "\n";*/

			//// 写入本地日志文件（验证数据是否接收成功）
			//std::ofstream logFile("dblclick_log.txt", std::ios::app);
			//if (logFile.is_open()) {
			//	logFile << logContent;
			//	logFile.close();
			//}

			 //3. 构造响应数据（要返回给扩展的内容）
			/*json response;
			response["status"] = "success";
			response["message"] = "C++ exe 已成功处理数据";
			response["processedData"] = {
				{"targetTag", targetTag},
				{"pageUrl", pageUrl},
				{"logPath", "dblclick_log.txt"}
			};*/

			// //4. 向扩展发送响应（按 Native Messaging 协议封装）
			//sendNativeMessage(response);
		}
	}
	catch (const std::exception& e) {
		// 异常处理，向扩展返回错误信息
		json errorResponse;
		errorResponse["status"] = "error";
		errorResponse["message"] = "C++ exe 处理失败：" + std::string(e.what());
		sendNativeMessage(errorResponse);
		return 1;
	}

	return 0;
}


int main()
{

}
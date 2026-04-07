//#include <winsock2.h>
//#include <windows.h>
//#include <process.h>
//#include <iostream>
//#include <string>
//#include <sstream>
//
//#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "crypt32.lib")  // Windows 加密库（自带）
//
//#define PORT 8899
//#define BUF_SIZE 4096
//
//using namespace std;
//
//// ============================
//// Windows 原生 SHA1
//// ============================
// 
//string Sha1(const string& input) {
//	HCRYPTPROV hProv = 0;
//	HCRYPTHASH hHash = 0;
//	BYTE hash[20];
//	DWORD hashLen = 20;
//
//	CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
//	CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
//	CryptHashData(hHash, (BYTE*)input.c_str(), input.size(), 0);
//	CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0);
//
//	CryptDestroyHash(hHash);
//	CryptReleaseContext(hProv, 0);
//
//	return string((char*)hash, hashLen);
//}
//
//// ============================
//// Windows 原生 Base64 编码
//// ============================
//string Base64Encode(const string& binary) {
//	DWORD len = 0;
//	CryptBinaryToStringA((BYTE*)binary.c_str(), binary.size(),
//		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &len);
//
//	string out;
//	out.resize(len);
//	CryptBinaryToStringA((BYTE*)binary.c_str(), binary.size(),
//		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0], &len);
//
//	return out;
//}
//
//// ============================
//// WebSocket 握手（关键）
//// ============================
//string WebSocketHandshake(const string& request) {
//	size_t keyStart = request.find("Sec-WebSocket-Key: ");
//	if (keyStart == string::npos) return "";
//
//	keyStart += 19;
//	size_t keyEnd = request.find("\r\n", keyStart);
//	string key = request.substr(keyStart, keyEnd - keyStart);
//
//	// WebSocket 固定 GUID
//	string guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
//	string accept = Base64Encode(Sha1(key + guid));
//
//	// 响应头
//	ostringstream oss;
//	oss << "HTTP/1.1 101 Switching Protocols\r\n"
//		<< "Upgrade: websocket\r\n"
//		<< "Connection: Upgrade\r\n"
//		<< "Sec-WebSocket-Accept: " << accept << "\r\n\r\n";
//
//	return oss.str();
//}
//
//// ============================
//// 接收 WebSocket 消息（解包）
//// ============================
//string WebSocketRecv(SOCKET sock) {
//	char buf[BUF_SIZE];
//	int recvLen = recv(sock, buf, BUF_SIZE, 0);
//	if (recvLen <= 0) return "";
//
//	BYTE* data = (BYTE*)buf;
//	int payloadLen = data[1] & 0x7F;
//	int maskOffset = 2;
//
//	if (payloadLen == 126) { maskOffset = 4; }
//	if (payloadLen == 127) { maskOffset = 10; }
//
//	BYTE mask[4];
//	memcpy(mask, data + maskOffset, 4);
//	char* payload = (char*)(data + maskOffset + 4);
//
//	for (int i = 0; i < payloadLen; i++) {
//		payload[i] ^= mask[i % 4];
//	}
//
//	return string(payload, payloadLen);
//}
//
//// ============================
//// 发送 WebSocket 消息（打包）
//// ============================
//void WebSocketSend(SOCKET sock, const string& msg) {
//	char frame[BUF_SIZE];
//	frame[0] = 0x81;
//	int len = msg.size();
//
//	if (len <= 125) {
//		frame[1] = len;
//		memcpy(frame + 2, msg.c_str(), len);
//		send(sock, frame, 2 + len, 0);
//	}
//}
//
//// ============================
//// 客户端线程
//// ============================
//unsigned __stdcall ClientThread(void* param) {
//	SOCKET sock = (SOCKET)param;
//	char buf[BUF_SIZE];
//
//	// 握手
//	recv(sock, buf, BUF_SIZE, 0);
//	string response = WebSocketHandshake(buf);
//	send(sock, response.c_str(), response.size(), 0);
//
//	// 循环收发消息
//	while (true) {
//		string msg = WebSocketRecv(sock);
//		if (msg.empty()) break;
//
//		cout << "[Firefox] " << msg << endl;
//
//		// 回复插件
//		WebSocketSend(sock, "守护进程已收到：" + msg);
//	}
//
//	closesocket(sock);
//	return 0;
//}
//
//// ============================
//// 守护进程主逻辑
//// ============================
//void RunDaemon() {
//	WSADATA wsa;
//	WSAStartup(MAKEWORD(2, 2), &wsa);
//
//	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	sockaddr_in addr = { AF_INET, htons(PORT), INADDR_ANY };
//
//	bind(server, (sockaddr*)&addr, sizeof(addr));
//	listen(server, 5);
//
//	cout << "守护进程已启动，WebSocket 端口：" << PORT << endl;
//
//	while (true) {
//		SOCKET client = accept(server, NULL, NULL);
//		_beginthreadex(NULL, 0, ClientThread, (void*)client, 0, NULL);
//	}
//}
//
//// ============================
//// 主入口
//// ============================
//int main() {
//	// 隐藏控制台（发布用）
//	//HWND hwnd = GetConsoleWindow();
//	//ShowWindow(hwnd, SW_HIDE);
//
//	RunDaemon();
//	return 0;
//}




#include <winsock.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8899
#define BUF_SIZE 4096 * 2

using namespace std;

// 精准接收指定长度的数据
string RecvExact(SOCKET sock, int exactSize) {
	string data;
	char buf[4096];
	int need = exactSize;

	while (need > 0) {
		int r = recv(sock, buf, min(need, 4096), 0);
		if (r <= 0) break;
		data.append(buf, r);
		need -= r;
	}
	return data;
}

string GetHttpBody(const string& data) {
	size_t pos = data.find("\r\n\r\n");
	if (pos == string::npos) return "";
	return data.substr(pos + 4);
}
int g_expected_length = 0;
int g_nBookmarkSize = 0;
unsigned __stdcall ClientThread(void* param) {
	SOCKET sock = (SOCKET)param;
	unique_ptr<char[]> uptrBuff;
	string sFullMsg;
	if (g_nBookmarkSize > 0)
	{
		uptrBuff.reset(new char[g_nBookmarkSize + 1]);
		int len = recv(sock, uptrBuff.get(), BUF_SIZE, 0);
		if (len >= g_nBookmarkSize + 1)
		{
			uptrBuff[g_nBookmarkSize] = 0;
		}
		else
		{
			uptrBuff[len] = 0;
		}
		sFullMsg = uptrBuff.get();
		string sDataMsg = GetHttpBody(sFullMsg);
		cout << "[Firefox] " << sDataMsg << endl;
	}
	else
	{
		char buf[BUF_SIZE];
		int len = recv(sock, buf, BUF_SIZE, 0);
		if (len >= BUF_SIZE)
		{
			buf[BUF_SIZE - 1] = 0;
		}
		else
		{
			buf[len] = 0;
		}
		sFullMsg = buf;
		string sDataMsg = GetHttpBody(sFullMsg);
		cout << "[Firefox] " << sDataMsg << endl;
		g_nBookmarkSize = strtoul(sDataMsg.c_str(), NULL, 16);
	}
	// 回复插件
	string resp = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
	send(sock, resp.c_str(), resp.size(), 0);

	closesocket(sock);
	return 0;
}

void RunDaemon() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	bind(server, (sockaddr*)&addr, sizeof(addr));
	listen(server, 5);

	cout << u8"守护进程已启动：127.0.0.1:" << PORT << endl;

	while (true) {
		SOCKET client = accept(server, NULL, NULL);
		_beginthreadex(0, 0, ClientThread, (void*)client, 0, 0);
	}
}

int main() {
	SetConsoleOutputCP(CP_UTF8);
	// 调试用：显示窗口
	RunDaemon();

	// 发布用：隐藏窗口
	//HWND hwnd = GetConsoleWindow();
	//ShowWindow(hwnd, SW_HIDE);
	//RunDaemon();
	return 0;
}
//
//#include <winsock2.h>
//#include <windows.h>
//#include <process.h>
//#include <iostream>
//#include <string>
//
//#pragma comment(lib, "ws2_32.lib")
//
//#define PORT 8899
//#define BUF_SIZE 4096
//
//using namespace std;
//
//unsigned __stdcall ClientThread(void* param) {
//	SOCKET sock = (SOCKET)param;
//	char buf[BUF_SIZE];
//
//	while (true) {
//		int len = recv(sock, buf, BUF_SIZE, 0);
//		if (len <= 0) break;
//
//		// 直接输出原始数据看看是什么
//		buf[len] = 0;
//		printf("原始数据: %s\n", buf);
//
//		// 回复插件
//		string reply = "已收到: ";
//		send(sock, reply.c_str(), reply.length(), 0);
//	}
//
//	closesocket(sock);
//	return 0;
//}
//
//void RunDaemon() {
//	WSADATA wsa;
//	WSAStartup(MAKEWORD(2, 2), &wsa);
//
//	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//
//	sockaddr_in addr = { 0 };
//	addr.sin_family = AF_INET;
//	addr.sin_port = htons(PORT);
//	addr.sin_addr.s_addr = INADDR_ANY;
//
//	bind(server, (sockaddr*)&addr, sizeof(addr));
//	listen(server, 5);
//
//	printf("守护进程已启动，端口 %d\n", PORT);
//
//	while (true) {
//		SOCKET client = accept(server, NULL, NULL);
//		_beginthreadex(0, 0, ClientThread, (void*)client, 0, 0);
//	}
//}
//
//int main() {
//	// 让控制台能显示UTF-8中文
//	RunDaemon();
//	return 0;
//}


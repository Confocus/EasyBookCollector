#include <winsock.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8899
#define BUF_SIZE 4096 * 2

using namespace std;

uint32_t GetHttpHeadLength(const string& data)
{
	// 找到头结束的位置 \r\n\r\n
	size_t pos = data.find("\r\n\r\n");

	if (pos == string::npos)
		return -1; // 没找到头

	// 头的总长度 = 结束位置 + 4（把 \r\n\r\n 也算进头里）
	return (int)(pos + 4);
}

string GetHttpBody(const string& data) {
	size_t pos = data.find("\r\n\r\n");
	if (pos == string::npos) return "";
	return data.substr(pos + 4);
}

uint32_t g_nBookmarkSize = 0;
uint32_t g_uRecvLen = 0;
unsigned __stdcall ClientThread(void* param) {
	SOCKET sock = (SOCKET)param;
	unique_ptr<char[]> uptrBuff;
	if (g_nBookmarkSize > 0)//todo:重构一下这里的代码；传输给server.exe；server.exe中重新解析这里的数据
	{
		BOOL bHeadCounted = FALSE;
		uptrBuff.reset(new char[g_nBookmarkSize + 1]());
		while (g_nBookmarkSize > 0)
		{
			unsigned int uLen = recv(sock, uptrBuff.get() + g_uRecvLen, g_nBookmarkSize, 0);
			if (uLen == 0)
			{
				break;
			}

			if (!bHeadCounted)
			{
				unsigned int uHeadLen = GetHttpHeadLength(uptrBuff.get());
				if (uHeadLen <= 0)
				{
					break;
				}
				g_nBookmarkSize += uHeadLen;
				bHeadCounted = TRUE;
			}

			g_uRecvLen += uLen;
			g_nBookmarkSize -= uLen;
		}
		uptrBuff[g_uRecvLen] = 0;
		string sFullMsg = uptrBuff.get();
		cout << "[Firefox] " << sFullMsg << endl;
		string sDataMsg = GetHttpBody(sFullMsg);
		cout << "[Firefox] " << sDataMsg << endl;
	}
	else
	{
		char szBuf[BUF_SIZE];
		uint32_t uLen = recv(sock, szBuf, BUF_SIZE, 0);
		if (uLen >= BUF_SIZE)
		{
			szBuf[BUF_SIZE - 1] = 0;
		}
		else
		{
			szBuf[uLen] = 0;
		}
		string sDataMsg = GetHttpBody(szBuf);
		cout << "[Firefox] " << sDataMsg << endl;

		//发送端要改成按字节数量发送，因为有的中文字符一个字符占3个字节
		//let encoder = new TextEncoder();
		//let dataBytes = encoder.encode(bookmarkText);
		//let dataLength = dataBytes.length.toString().padStart(8, '0');

		g_nBookmarkSize = strtoul(sDataMsg.c_str(), NULL, 10);
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
	RunDaemon();

	return 0;
}
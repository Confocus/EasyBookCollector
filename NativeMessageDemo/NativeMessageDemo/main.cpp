//#include <winsock.h>
//#include <windows.h>
//#include <process.h>
//#include <iostream>
//#include <string>

//#pragma comment(lib, "ws2_32.lib")

//#define PORT 8899
//#define BUF_SIZE 4096 * 2
//
//using namespace std;
//
//uint32_t GetHttpHeadLength(const string& data)
//{
//	// 找到头结束的位置 \r\n\r\n
//	size_t pos = data.find("\r\n\r\n");
//
//	if (pos == string::npos)
//		return -1; // 没找到头
//
//	// 头的总长度 = 结束位置 + 4（把 \r\n\r\n 也算进头里）
//	return (int)(pos + 4);
//}
//
//string GetHttpBody(const string& data) {
//	size_t pos = data.find("\r\n\r\n");
//	if (pos == string::npos) return "";
//	return data.substr(pos + 4);
//}
//
//uint32_t g_nBookmarkSize = 0;
//uint32_t g_uRecvLen = 0;
//unsigned __stdcall RecvAllBookmarksThread(void* param) {
//	SOCKET sock = (SOCKET)param;
//	unique_ptr<char[]> uptrBuff;
//	do {
//		if (g_nBookmarkSize > 0)//todo:重构一下这里的代码；传输给server.exe；server.exe中重新解析这里的数据
//		{
//			BOOL bHeadCounted = FALSE;
//			uptrBuff.reset(new char[g_nBookmarkSize + 1]());
//			while (g_nBookmarkSize > 0)
//			{
//				unsigned int uLen = recv(sock, uptrBuff.get() + g_uRecvLen, g_nBookmarkSize, 0);
//				if (uLen == 0)
//				{
//					break;
//				}
//
//				if (!bHeadCounted)
//				{
//					unsigned int uHeadLen = GetHttpHeadLength(uptrBuff.get());
//					if (uHeadLen <= 0)
//					{
//						break;
//					}
//					g_nBookmarkSize += uHeadLen;
//					bHeadCounted = TRUE;
//				}
//
//				g_uRecvLen += uLen;
//				g_nBookmarkSize -= uLen;
//			}
//			uptrBuff[g_uRecvLen] = 0;
//			string sFullMsg = uptrBuff.get();
//			cout << "[Firefox] " << sFullMsg << endl;
//			string sDataMsg = GetHttpBody(sFullMsg);
//			cout << "[Firefox] " << sDataMsg << endl;
//		}
//		else
//		{
//			char szBuf[BUF_SIZE];
//			uint32_t uLen = recv(sock, szBuf, BUF_SIZE, 0);
//			if (uLen >= BUF_SIZE)
//			{
//				szBuf[BUF_SIZE - 1] = 0;
//			}
//			else
//			{
//				szBuf[uLen] = 0;
//			}
//			string sDataMsg = GetHttpBody(szBuf);
//			cout << "[Firefox] " << sDataMsg << endl;
//
//			//发送端要改成按字节数量发送，因为有的中文字符一个字符占3个字节
//			//let encoder = new TextEncoder();
//			//let dataBytes = encoder.encode(bookmarkText);
//			//let dataLength = dataBytes.length.toString().padStart(8, '0');
//
//			g_nBookmarkSize = strtoul(sDataMsg.c_str(), NULL, 10);
//			if (0 == g_nBookmarkSize)
//			{
//				break;
//			}
//		}
//	} while (0);
//	
//	// 回复插件
//	string resp = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
//	send(sock, resp.c_str(), resp.size(), 0);
//
//	closesocket(sock);
//	return 0;
//}
//
//std::string GetCmdFromGUI()
//{
//	//todo:这里暂时写死返回的命令
//	return "";
//}
//
//void RunDaemon() {
//	WSADATA wsa;
//	WSAStartup(MAKEWORD(2, 2), &wsa);
//
//	SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	sockaddr_in addr = { 0 };
//	addr.sin_family = AF_INET;
//	addr.sin_port = htons(PORT);
//	addr.sin_addr.s_addr = INADDR_ANY;
//
//	bind(server, (sockaddr*)&addr, sizeof(addr));
//	listen(server, 5);
//
//	cout << u8"守护进程已启动：127.0.0.1:" << PORT << endl;
//
//	while (true) //todo:在这里循环等待发过来的消息？
//	{
//		//WaitForSingleObject()//如果收到了send reload-bookmarks的Event就去链接accept，然后reload一次
//		//没有启动浏览器或加载插件的时候就一直在这里等待
//		SOCKET client = accept(server, NULL, NULL);
//
//		//todo：其它的命令也在这里判断
//		//todo:这里判断如果外部命令给与的是reload-bookmarks
//		std::cout << u8"✅ 插件已连接！\n";
//		const char* cmd = "reload-bookmarks";
//
//		/*char buf[4096];
//		int recvLen = recv(client, buf, sizeof(buf) - 1, 0);
//		if (recvLen <= 0) { closesocket(client); return; }
//		buf[recvLen] = 0;*/
//
//		const char* resp =
//			"HTTP/1.1 200 OK\r\n"
//			"Access-Control-Allow-Origin: *\r\n"
//			"Content-Type: text/plain\r\n"
//			"Content-Length: 16\r\n"
//			"\r\n"
//			"reload-bookmarks";
//
//		send(client, resp, strlen(resp), 0);
//
//		//todo：发送完reload-bookmarks命令自然就是接收bookmarks了
//		//_beginthreadex(0, 0, RecvAllBookmarksThread, (void*)client, 0, 0);
//		Sleep(1 * 1000);
//		closesocket(client);
//	}
//
//	/*closesocket(server);
//	WSACleanup();*/
//}
//
//int main() {
//	SetConsoleOutputCP(CP_UTF8);
//	//todo：不用exe来控制Firefox加载插件，而是让Firefox一启动就自己主动加载插件，然后尝试与exe建立连接
//	RunDaemon();
//
//	return 0;
//}
#define _WINSOCKAPI_

#include <windows.h>
//#include <winsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
//#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define PORT 8899

// 纯C实现 SHA1  【无依赖】
void SHA1_Simple(const char* input, char* outSHA1)
{
	unsigned char hash[20];
	HCRYPTPROV hProv;
	HCRYPTHASH hHash;

	CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, 0);
	CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash);
	CryptHashData(hHash, (const BYTE*)input, strlen(input), 0);
	DWORD dwLen = 20;
	CryptGetHashParam(hHash, HP_HASHVAL, hash, &dwLen, 0);

	memcpy(outSHA1, hash, 20);

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
}

// 纯C实现 Base64 【无依赖】
void Base64_Encode(const unsigned char* src, int len, char* dest)
{
	static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i, j;
	for (i = 0, j = 0; i < len; i += 3, j += 4)
	{
		dest[j] = table[(src[i] & 0xFC) >> 2];
		dest[j + 1] = table[((src[i] & 0x03) << 4) | ((src[i + 1] & 0xF0) >> 4)];
		dest[j + 2] = (i + 1 < len) ? table[((src[i + 1] & 0x0F) << 2) | ((src[i + 2] & 0xC0) >> 6)] : '=';
		dest[j + 3] = (i + 2 < len) ? table[src[i + 2] & 0x3F] : '=';
	}
	dest[j] = 0;
}

// 计算 WebSocket Accept
void ComputeWebSocketAccept(const char* key, char* out)
{
	char buf[1024];
	const char* guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	wsprintfA(buf, "%s%s", key, guid);

	char sha1[20];
	SHA1_Simple(buf, sha1);

	Base64_Encode((unsigned char*)sha1, 20, out);
}

// 发送 WebSocket 消息
void SendWebSocketMsg(SOCKET s, const char* msg)
{
	char buf[1024];
	int len = lstrlenA(msg);

	buf[0] = 0x81;
	buf[1] = len;
	memcpy(buf + 2, msg, len);

	send(s, buf, 2 + len, 0);
}

int main()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	/*SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == INVALID_SOCKET) {
		printf("socket failed: %d\n", WSAGetLastError());
	}
	int opt = 1;
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));*/

	/*struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");*/
	SOCKET server = socket(AF_INET6, SOCK_STREAM, 0);

	sockaddr_in6 addr{};
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(PORT);
	addr.sin6_addr = in6addr_any;

	//bind(server, (sockaddr*)&addr, sizeof(addr));
	if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) 
	{
		printf("bind failed: %d\n", WSAGetLastError());
		return 1;
	}

	if (listen(server, 5) == SOCKET_ERROR) 
	{
		printf("listen failed: %d\n", WSAGetLastError());
		return 1;
	}

	printf(u8"WebSocket 服务已启动: ws://127.0.0.1:%d\n", PORT);

	while (1)
	{
		SOCKET client = accept(server, NULL, NULL);
		printf("客户端已连接\n");

		char recvBuf[4096];
		recv(client, recvBuf, 4096, 0);

		char* keyPos = strstr(recvBuf, "Sec-WebSocket-Key: ");
		if (!keyPos) { closesocket(client); continue; }

		char key[256];
		sscanf_s(keyPos + 19, "%[^\r\n]", key, _countof(key));

		char accept[256];
		ComputeWebSocketAccept(key, accept);

		char response[1024];
		wsprintfA(response,
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: %s\r\n"
			"\r\n", accept);

		send(client, response, lstrlenA(response), 0);
		printf("握手成功！\n");

		// 发送命令给浏览器
		SendWebSocketMsg(client, "reload-bookmarks");
		printf("已发送命令：reload-bookmarks\n");

	}

	return 0;
}

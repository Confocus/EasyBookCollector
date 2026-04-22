
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

	/*closesocket(server);
	WSACleanup();*/
//}
////
////int main() {
////	SetConsoleOutputCP(CP_UTF8);
////	//todo：不用exe来控制Firefox加载插件，而是让Firefox一启动就自己主动加载插件，然后尝试与exe建立连接
////	RunDaemon();
////
////	return 0;
//}
#define _WINSOCKAPI_

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <process.h>

using namespace std;
#pragma comment(lib, "ws2_32.lib")

#define PORT 8899
#define BUF_SIZE 4096 * 2

std::string GetCmdFromRemote()
{
	//todo:这里暂时写死返回的命令
	return "reload-bookmarks";
}

bool RecvWebSocketData(SOCKET sock, char* outBuf, int& outLen)
{
	BYTE header[2];
	if (recv(sock, (char*)header, 2, 0) <= 0) return false;

	// 1. 提取帧类型（文本=0x01）
	BYTE fin = header[0] & 0x80;
	BYTE opcode = header[0] & 0x0F;

	// 2. 必须有掩码（浏览器发的一定有）
	bool hasMask = (header[1] & 0x80) != 0;
	uint64_t payloadLen = header[1] & 0x7F;

	// 3. 处理长格式长度（关键！你发书签必走这里！）
	if (payloadLen == 126)
	{
		// 2字节扩展长度 → 必须用 ntohs 转字节序！
		uint16_t len16;
		recv(sock, (char*) & len16, 2, 0);
		payloadLen = ntohs(len16); // ✅ 这才是对的！
	}
	else if (payloadLen == 127)
	{
		// 你发书签永远走不到这里！！！
		// 如果你强行读8字节，必错！
		uint64_t len64;
		recv(sock, (char*)&len64, 8, 0);
		payloadLen = _byteswap_uint64(len64); // 必须反转
	}

	// 4. 读取掩码
	BYTE mask[4] = { 0 };
	if (hasMask) recv(sock, (char*)mask, 4, 0);

	// 5. 读取真实内容
	if (payloadLen > 4096) payloadLen = 4096; // 防溢出
	recv(sock, outBuf, (int)payloadLen, 0);

	// 6. 解密！！！
	for (uint64_t i = 0; i < payloadLen; i++)
		outBuf[i] ^= mask[i % 4];

	outLen = (int)payloadLen;
	outBuf[outLen] = 0; // 字符串结束符
	cout << outBuf << endl;
	return true;
}


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
unsigned __stdcall RecvAllBookmarksThread(void* param) 
{
	SOCKET sock = (SOCKET)param;
	unique_ptr<char[]> uptrBuff;
	do {
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
			char szBuf[BUF_SIZE] = {0};
			//uint32_t uLen = recv(sock, szBuf, BUF_SIZE, 0);
			/*if (uLen >= BUF_SIZE)
			{
				szBuf[BUF_SIZE - 1] = 0;
			}
			else
			{
				szBuf[uLen] = 0;
			}*/
			char recvBuf[4096];
			int dataLen = 0;
			RecvWebSocketData(sock, recvBuf, dataLen);
			string sDataMsg = GetHttpBody(szBuf);
			cout << "[Firefox] " << sDataMsg << endl;

			//发送端要改成按字节数量发送，因为有的中文字符一个字符占3个字节
			//let encoder = new TextEncoder();
			//let dataBytes = encoder.encode(bookmarkText);
			//let dataLength = dataBytes.length.toString().padStart(8, '0');

			g_nBookmarkSize = strtoul(sDataMsg.c_str(), NULL, 10);
			if (0 == g_nBookmarkSize)
			{
				break;
			}
		}
	} while (0);
	
	// 回复插件
	string resp = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\n";
	send(sock, resp.c_str(), resp.size(), 0);

	closesocket(sock);
	return 0;
}

bool SHA1_Simple(const char* input, char* outSHA1)
{
	HCRYPTPROV hProv = NULL;
	HCRYPTHASH hHash = NULL;
	DWORD dwLen = 20;
	BOOL bResult = FALSE;

	// ✅ 正确写法：必须加 CRYPT_VERIFYCONTEXT
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		goto cleanup;

	// ✅ 创建 SHA1
	if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
		goto cleanup;

	// ✅ 输入数据（用 input.size() 而不是 strlen）
	if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input), strlen(input), 0))
		goto cleanup;

	// ✅ 获取结果（20字节）
	if (!CryptGetHashParam(hHash, HP_HASHVAL, reinterpret_cast<BYTE*>(outSHA1), &dwLen, 0))
		goto cleanup;

	bResult = TRUE;

cleanup:
	if (hHash) CryptDestroyHash(hHash);
	if (hProv) CryptReleaseContext(hProv, 0);
	return bResult;
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
inline char HexChar(unsigned char c) {
	return c < 10 ? '0' + c : 'a' + c - 10;
}

void BinToHexString(char* bin, int binLen, char* outStr) {
	for (int i = 0; i < binLen; i++) {
		outStr[i * 2] = HexChar((bin[i] >> 4) & 0x0F);
		outStr[i * 2 + 1] = HexChar(bin[i] & 0x0F);
	}
	outStr[binLen * 2] = 0; // 最后加结束符
}

void ComputeWebSocketAccept(const char* key, char* out)
{
	char buf[1024] = {0};
	const char* guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	wsprintfA(buf, "%s%s", key, guid);

	char sha1[20] = {0};
	SHA1_Simple(buf, sha1);
	// 输出：字符串
	//char str[100] = { 0 };
	//BinToHexString(sha1, 20, str);

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

#include <wincrypt.h>
#pragma comment(lib, "Crypt32.lib")

std::string CalcWebSocketAccept(const std::string& key)
{
	const std::string guid =
		"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	std::string input = key + guid;

	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;

	BYTE hash[20];
	DWORD hashLen = 20;

	CryptAcquireContext(
		&hProv, NULL, NULL,
		PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT
	);

	CryptCreateHash(
		hProv,
		CALG_SHA1,
		0, 0,
		&hHash
	);

	CryptHashData(
		hHash,
		(BYTE*)input.c_str(),
		(DWORD)input.size(),
		0
	);

	CryptGetHashParam(
		hHash,
		HP_HASHVAL,
		hash,
		&hashLen,
		0
	);

	DWORD outLen = 64;
	char base64[64] = { 0 };

	CryptBinaryToStringA(
		hash,
		20,
		CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
		base64,
		&outLen
	);

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	return base64;
}

namespace uWS {

	struct WebSocketHandshake {
		template <int N, typename T>
		struct static_for {
			void operator()(uint32_t* a, uint32_t* b) {
				static_for<N - 1, T>()(a, b);
				T::template f<N - 1>(a, b);
			}
		};

		template <typename T>
		struct static_for<0, T> {
			void operator()(uint32_t* a, uint32_t* hash) {}
		};

		template <int state>
		struct Sha1Loop {
			static inline uint32_t rol(uint32_t value, size_t bits) { return (value << bits) | (value >> (32 - bits)); }
			static inline uint32_t blk(uint32_t b[16], size_t i) {
				return rol(b[(i + 13) & 15] ^ b[(i + 8) & 15] ^ b[(i + 2) & 15] ^ b[i], 1);
			}

			template <int i>
			static inline void f(uint32_t* a, uint32_t* b) {
				switch (state) {
				case 1:
					a[i % 5] += ((a[(3 + i) % 5] & (a[(2 + i) % 5] ^ a[(1 + i) % 5])) ^ a[(1 + i) % 5]) + b[i] + 0x5a827999 + rol(a[(4 + i) % 5], 5);
					a[(3 + i) % 5] = rol(a[(3 + i) % 5], 30);
					break;
				case 2:
					b[i] = blk(b, i);
					a[(1 + i) % 5] += ((a[(4 + i) % 5] & (a[(3 + i) % 5] ^ a[(2 + i) % 5])) ^ a[(2 + i) % 5]) + b[i] + 0x5a827999 + rol(a[(5 + i) % 5], 5);
					a[(4 + i) % 5] = rol(a[(4 + i) % 5], 30);
					break;
				case 3:
					b[(i + 4) % 16] = blk(b, (i + 4) % 16);
					a[i % 5] += (a[(3 + i) % 5] ^ a[(2 + i) % 5] ^ a[(1 + i) % 5]) + b[(i + 4) % 16] + 0x6ed9eba1 + rol(a[(4 + i) % 5], 5);
					a[(3 + i) % 5] = rol(a[(3 + i) % 5], 30);
					break;
				case 4:
					b[(i + 8) % 16] = blk(b, (i + 8) % 16);
					a[i % 5] += (((a[(3 + i) % 5] | a[(2 + i) % 5]) & a[(1 + i) % 5]) | (a[(3 + i) % 5] & a[(2 + i) % 5])) + b[(i + 8) % 16] + 0x8f1bbcdc + rol(a[(4 + i) % 5], 5);
					a[(3 + i) % 5] = rol(a[(3 + i) % 5], 30);
					break;
				case 5:
					b[(i + 12) % 16] = blk(b, (i + 12) % 16);
					a[i % 5] += (a[(3 + i) % 5] ^ a[(2 + i) % 5] ^ a[(1 + i) % 5]) + b[(i + 12) % 16] + 0xca62c1d6 + rol(a[(4 + i) % 5], 5);
					a[(3 + i) % 5] = rol(a[(3 + i) % 5], 30);
					break;
				case 6:
					b[i] += a[4 - i];
				}
			}
		};

		/**
		 * sha1 函数的实现
		 */
		static inline void sha1(uint32_t hash[5], uint32_t b[16]) {
			uint32_t a[5] = { hash[4], hash[3], hash[2], hash[1], hash[0] };
			static_for<16, Sha1Loop<1>>()(a, b);
			static_for<4, Sha1Loop<2>>()(a, b);
			static_for<20, Sha1Loop<3>>()(a, b);
			static_for<20, Sha1Loop<4>>()(a, b);
			static_for<20, Sha1Loop<5>>()(a, b);
			static_for<5, Sha1Loop<6>>()(a, hash);
		}

		/**
		 * base64 编码函数
		 */
		static inline void base64(unsigned char* src, char* dst) {
			const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			for (int i = 0; i < 18; i += 3) {
				*dst++ = b64[(src[i] >> 2) & 63];
				*dst++ = b64[((src[i] & 3) << 4) | ((src[i + 1] & 240) >> 4)];
				*dst++ = b64[((src[i + 1] & 15) << 2) | ((src[i + 2] & 192) >> 6)];
				*dst++ = b64[src[i + 2] & 63];
			}
			*dst++ = b64[(src[18] >> 2) & 63];
			*dst++ = b64[((src[18] & 3) << 4) | ((src[19] & 240) >> 4)];
			*dst++ = b64[((src[19] & 15) << 2)];
			*dst++ = '=';
		}

	public:
		/**
		 * 生成 Sec-WebSocket-Accept 算法
		 * @param input 对端传过来的Sec-WebSocket-Key值
		 * @param output 存放生成的 Sec-WebSocket-Accept 值
		 */
		static inline void generate(const char input[24], char output[28]) {
			uint32_t b_output[5] = {
				0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0
			};
			uint32_t b_input[16] = {
				0, 0, 0, 0, 0, 0, 0x32353845, 0x41464135, 0x2d453931, 0x342d3437, 0x44412d39,
				0x3543412d, 0x43354142, 0x30444338, 0x35423131, 0x80000000
			};

			for (int i = 0; i < 6; i++) {
				b_input[i] = (input[4 * i + 3] & 0xff) | (input[4 * i + 2] & 0xff) << 8 | (input[4 * i + 1] & 0xff) << 16 | (input[4 * i + 0] & 0xff) << 24;
			}
			sha1(b_output, b_input);
			uint32_t last_b[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 480 };
			sha1(b_output, last_b);
			for (int i = 0; i < 5; i++) {
				uint32_t tmp = b_output[i];
				char* bytes = (char*)&b_output[i];
				bytes[3] = tmp & 0xff;
				bytes[2] = (tmp >> 8) & 0xff;
				bytes[1] = (tmp >> 16) & 0xff;
				bytes[0] = (tmp >> 24) & 0xff;
			}
			base64((unsigned char*)b_output, output);
		}
	};
}

VOID SendCmd()
{

}

unsigned __stdcall SendAndRecvCommand(void* param)
{
	//------------------------------------------------
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	SOCKET server = socket(AF_INET6, SOCK_STREAM, 0);

	sockaddr_in6 addr{};
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(PORT);
	addr.sin6_addr = in6addr_any;

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
	//过往的要在accept处等很久的原因：失败的次数太多了导致FireFox尝试重连的间隔越来越长
	
	while (1)
	{
		printf("%llu waiting accept\n", GetTickCount64());
		SOCKET client = accept(server, NULL, NULL);
		printf("%llu accept ok\n", GetTickCount64());

		char recvBuf[4096] = { 0 };
		recv(client, recvBuf, 4096, 0);
		cout << recvBuf << endl;

		char* keyPos = strstr(recvBuf, "Sec-WebSocket-Key: ");
		if (!keyPos) 
		{ 
			closesocket(client); 
			continue; 
		}

		char key[25] = { 0 };
		sscanf_s(keyPos + 19, "%[^\r\n]", key, _countof(key));
		cout << key << endl;

		char output[29] = { 0 };
		uWS::WebSocketHandshake temp;
		temp.generate(key, output);

		char response[1024] = { 0 };
		wsprintfA(response,
			"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: %s\r\n"
			"Sec-WebSocket-Protocol: chat\r\n"
			"\r\n", output);

		cout << response << endl;
		if (send(client, response, lstrlenA(response), 0) == SOCKET_ERROR)
		{
			printf("send failed: %d\n", WSAGetLastError());
		}
		printf("握手成功！\n");

		if (strcmp(GetCmdFromRemote().c_str(), "reload-bookmarks") == 0)
		{
			SendWebSocketMsg(client, "reload-bookmarks");
			printf("已发送命令：reload-bookmarks\n");
			RecvAllBookmarksThread((void*)client);

		}
		else
		{
		}
		// 发送命令给浏览器
		closesocket(client);

	}
	closesocket(server);

	// 可选关（程序退出前调用一次就行）
	WSACleanup();
}

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	_beginthreadex(0, 0, SendAndRecvCommand, (void*)NULL, 0, 0);
	//todo:等待接收外部通知
	getchar();
	return 0;
}
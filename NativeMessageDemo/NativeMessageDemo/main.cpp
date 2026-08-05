
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

#define PORT			8899
#define MAX_BUF_SIZE	4096 * 2
#define PIPE_NAME_BOOKMARK_TRANS	L"\\\\.\\pipe\\BookmarkTransPipe"
#define EVENT_NAME_CONNECT_PIPE	L"{A1418B8A-7998-4262-9D44-47E607653E93}\ConnectPipe"
#define EVENT_NAME_DISCONNECT_PIPE	L"{4E17318B-F76A-448B-8401-42085E3AC90D}\DisconnectPipe"
#define EVENT_NAME_SENT_RECV_CMD	L"{31E3A6F1-105A-45D9-8E73-79CE24064F5C}\SendRecvCmd"
#define EVENT_NAME_RESPONSE	L"{A7486818-B995-4F67-BA45-834BE0B980EC}\Response"

#define STRING_RELOAD_BOOKMARKS	"reload-bookmarks"
#define UID_RELOAD_BOOKMARKS	1
#define STRING_ADD_ACTIVE_TAB	"AddActiveTab"
#define UID_ADD_ACTIVE_TAB		2

std::wstring CharToWchar(const char* pSrc, UINT codepage = CP_UTF8)
{
	if (pSrc == nullptr) return L"";
	int len = MultiByteToWideChar(codepage, 0, pSrc, -1, nullptr, 0);
	if (len <= 0) return L"";
	std::wstring buf(len, 0);
	MultiByteToWideChar(codepage, 0, pSrc, -1, &buf[0], len);
	return buf;
}


std::string GetCmdFromRemote(HANDLE hPipe)
{
	char szRecvCmd[4096] = { 0 };
	DWORD readLen = 0;
	//从管道读取命令
	BOOL bRet = ReadFile(hPipe, szRecvCmd, 4095, &readLen, nullptr);
	if (!bRet || readLen == 0)
	{
		printf("GetCmdFromRemote 读取命令失败\n");
		return "";
	}
	std::wstring wsRecvCmd = CharToWchar(szRecvCmd);
	printf("GetCmdFromRemote 读取命令成功：%s\n", wsRecvCmd);
	return szRecvCmd;
}

BOOL RecvWebSocketData(SOCKET sock, shared_ptr<char[]> &spRecvBuf, int64_t& outLen)
{
	BYTE header[2];
	if (SOCKET_ERROR == recv(sock, (char*)header, 2, 0) )
	{
		return FALSE;
	}

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
		if (SOCKET_ERROR == recv(sock, (char*)&len16, 2, 0))
		{
			return FALSE;
		}
		payloadLen = ntohs(len16); // ✅ 这才是对的！
	}
	else if (payloadLen == 127)
	{
		// 你发书签永远走不到这里！！！
		// 如果你强行读8字节，必错！
		uint64_t len64;
		if (SOCKET_ERROR == recv(sock, (char*)&len64, 8, 0))
		{
			return FALSE;
		}
		payloadLen = _byteswap_uint64(len64); // 必须反转
	}

	// 4. 读取掩码
	BYTE mask[4] = { 0 };
	if (hasMask)
	{
		if (SOCKET_ERROR == recv(sock, (char*)mask, 4, 0))
		{
			return FALSE;
		}
	}
	// 5. 读取真实内容
	spRecvBuf.reset(new char[payloadLen + 1]());
	//if (payloadLen > 4096) payloadLen = 4096; // 防溢出
	if (SOCKET_ERROR == recv(sock, spRecvBuf.get(), (int)payloadLen, 0))
	{
		return FALSE;
	}

	// 6. 解密！！！
	for (uint64_t i = 0; i < payloadLen; i++)
	{
		spRecvBuf[i] ^= mask[i % 4];
	}

	spRecvBuf[payloadLen] = 0; // 字符串结束符
	outLen = payloadLen;
	//暂时不输出
	//cout << spRecvBuf << endl;
	return TRUE;
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
BOOL SendWebSocketMsg(SOCKET sock, const char* msg)
{
	char buf[1024];
	int len = lstrlenA(msg);
	buf[0] = 0x81;
	buf[1] = len;
	memcpy(buf + 2, msg, len);

	if (SOCKET_ERROR == send(sock, buf, 2 + len, 0))
	{
		cout << "send 失败！错误码：" << WSAGetLastError() << endl;
		closesocket(sock);
		return FALSE;
	}

	return TRUE;
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

BOOL TransferDataToGUI(HANDLE hPipe, std::shared_ptr<char[]> spData, int64_t nDataLen)
{
	HANDLE hConnectPipeEvent = NULL;
	HANDLE hResponse = NULL;
	printf("调用TransferBookMarksToGUI向GUI发送数据\n");
	DWORD dwWriteLen = 0;
	do 
	{
		//todo：这里是封装成一个大的数据包好，还是分两次发送好？
		std::string sDataLen = std::to_string(nDataLen);
		//if (!WriteFile(hPipe, sDataLen.c_str(), strlen(sDataLen.c_str()), &dwWriteLen, NULL))
		if (!WriteFile(hPipe, &nDataLen, sizeof(nDataLen), &dwWriteLen, NULL))
		{
			printf("发送长度写管道失败:%d\n", GetLastError());
			printf("Write %d %dbytes\n", nDataLen, dwWriteLen);
			return FALSE;
			break;
		}

		//发送数据
		if (!WriteFile(hPipe, spData.get(), nDataLen, &dwWriteLen, NULL))
		{
			printf("发送数据写管道失败:%d\n", GetLastError());
			printf("Write %d %dbytes\n", nDataLen, dwWriteLen);
			return FALSE;
			break;
		}
		printf("Write %d %dbytes\n", nDataLen, dwWriteLen);
		//通知对方可以接收response了
		/*hResponse = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			EVENT_NAME_RESPONSE
		);
		if (hResponse == NULL)
		{
			break;
		}
		SetEvent(hResponse);*/

		// todo:接收回复，确认这个回复是有必要的吗？
		/*char buf[4096] = { 0 };
		DWORD len = 0;
		ReadFile(hPipe, buf, 4096, &len, NULL);
		printf("服务器回复：%s\n", buf);*/
	} while (FALSE);

	/*if (hConnectPipeEvent)
	{
		CloseHandle(hConnectPipeEvent);
	}*/

	/*if (hPipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPipe);
	}*/

	/*if (hResponse)
	{
		CloseHandle(hResponse);
	}*/

	return TRUE;
}

BOOL SendAndRecvCommandInner(SOCKET client, HANDLE hPipe, std::string_view command)
{
	//todo：校验comand参数的合法性
	printf("读取到命令：%s\n",command.data());
	if (!SendWebSocketMsg(client, STRING_RELOAD_BOOKMARKS))
	{
		return FALSE;
	}

	int64_t nDataLen = 0;
	shared_ptr<char[]> spRecvBuf;
	if (!RecvWebSocketData(client, spRecvBuf, nDataLen))
	{
		return FALSE;
	}
	//todo：现在用的是Firefox 远程调试 CDP模式；比较模式Native Messaging看看有什么区别
	//通信传给GUI.exe
	if (!TransferDataToGUI(hPipe, spRecvBuf, nDataLen))
	{
		return FALSE;
	}

	return TRUE;
}

unsigned __stdcall SendAndRecvCommand(void* param)
{
	char szResponse[MAX_BUF_SIZE] = { 0 };
	BOOL bSucc = FALSE;
	char recvBuf[MAX_BUF_SIZE] = { 0 };
	char* keyPos = nullptr;
	char szKey[25] = { 0 };
	char szOutput[29] = { 0 };
	HANDLE hConnectPipeEvent = NULL;
	HANDLE hDisconnectPipeEvent = NULL;
	HANDLE hPipe = INVALID_HANDLE_VALUE;

	setlocale(LC_ALL, "chs");
	SetConsoleOutputCP(CP_UTF8);

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
		return 0;
	}

	if (listen(server, 5) == SOCKET_ERROR)
	{
		printf("listen failed: %d\n", WSAGetLastError());
		return 0;
	}
	printf("WebSocket 服务已启动: ws://127.0.0.1:%d\n", PORT);
	//注意：过往的要在accept处等很久的原因：失败的次数太多了导致FireFox尝试重连的间隔越来越长
	//与Firefox建立连接
	SOCKET client = accept(server, NULL, NULL);
	if (client == INVALID_SOCKET)
	{
		int err = WSAGetLastError();

		printf("accept failed, error = %d\n", err);
		return 0;
	}

	if (SOCKET_ERROR == recv(client, recvBuf, MAX_BUF_SIZE, 0))
	{
		return 0;
	}

	keyPos = strstr(recvBuf, "Sec-WebSocket-Key: ");
	if (!keyPos)
	{
		return 0;
	}

	sscanf_s(keyPos + 19, "%[^\r\n]", szKey, _countof(szKey));
	cout << szKey << endl;

	uWS::WebSocketHandshake temp;
	temp.generate(szKey, szOutput);

	wsprintfA(szResponse,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
		"Sec-WebSocket-Protocol: chat\r\n"
		"\r\n", szOutput);

	cout << szResponse << endl;
	if (send(client, szResponse, lstrlenA(szResponse), 0) == SOCKET_ERROR)
	{
		return 0;
	}

	//循环读取发送过来的命令
	while (TRUE)
	{
		//todo:相互通知创建管道
		//等待对方创建好管道，得到通知就可以连接
		printf("尝试连接\n");
		hConnectPipeEvent = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			EVENT_NAME_CONNECT_PIPE
		);

		if (hConnectPipeEvent == NULL)
		{
			break;
		}

		//确保管道是已经被创建好了的
		printf("等待管道连接\n");
		if (WAIT_OBJECT_0 != WaitForSingleObject(hConnectPipeEvent, INFINITE))
		{
			break;
		}
		printf("等待到管道连接信号\n");
		hDisconnectPipeEvent = CreateEvent(
			NULL,
			FALSE,
			FALSE,
			EVENT_NAME_DISCONNECT_PIPE
		);

		if (hDisconnectPipeEvent == NULL)
		{
			break;
		}

		// 连接管道（这就是打开管道的意思）
		printf("打开管道\n");
		HANDLE hPipe = CreateFile(
			PIPE_NAME_BOOKMARK_TRANS,                // 管道名称
			GENERIC_READ | GENERIC_WRITE,  // 可读可写（双向）
			0,                        // 独占模式（不能共享）
			NULL,                     // 安全属性
			OPEN_EXISTING,            // 必须是 OPEN_EXISTING
			0,                        // 无特殊属性
			NULL
		);

		// 判断是否连接成功
		if (hPipe == INVALID_HANDLE_VALUE)
		{
			printf("打开管道失败\n");
			break;
		}

		printf("读取管道中的命令\n");
		std::string command = GetCmdFromRemote(hPipe);
		if (!SendAndRecvCommandInner(client, hPipe, command))
		{
			goto ERROR_POINT;
		}

		//收到GUI处理完毕的通知才关闭pipe连接
		printf("等待关闭管道:%d\n", GetLastError());
		if (WAIT_OBJECT_0 != WaitForSingleObject(hDisconnectPipeEvent, INFINITE))
		{
			break;
		}
		printf("等待到关闭管道:%d\n", GetLastError());

		CloseHandle(hDisconnectPipeEvent);
		CloseHandle(hConnectPipeEvent);

		bSucc = TRUE;

	ERROR_POINT:
		if (!bSucc)
		{
			//todo:如果失败了通知Firefox再次尝试发送，处理client端的代码
			cout << "send 失败！错误码：" << WSAGetLastError() << endl;
			SendWebSocketMsg(client, "retry");
		}
	}
	
	/*if (hRecvCmdEvent)
	{
		CloseHandle(hRecvCmdEvent);
	}*/

	if (hConnectPipeEvent)
	{
		CloseHandle(hConnectPipeEvent);
	}

	if (hDisconnectPipeEvent)
	{
		CloseHandle(hDisconnectPipeEvent);
	}

	closesocket(client);
	closesocket(server);
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
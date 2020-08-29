#pragma once
#include <Windows.h>
#include <list>
#include <string>
#include <iostream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

//#define  __WS2

#ifdef __WS2
#include <WS2tcpip.h>
#endif

////////////使用前调用InitWinSocket()进行初始化/////////
inline bool InitWinSocket() {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
	return true;
}




#define  MAX_CONNECT_NUM  20
#define  SOCKET_REV_BUF_SIZE     1024

#define  GUID_LEN    16
#define  NewMsgMarkStr    "4A6959616E674C696E6DB62435625348"
#define  NewMsgMarkStrEnd "414B6D9D2AA1C14A6959616E674C696E"
#define  NewMsg_Size_LEN  4
#define  CMD_LEN          4

const int   Pos_cmd = GUID_LEN + NewMsg_Size_LEN;
const int   Pos_msg = Pos_cmd + CMD_LEN;
const int   Pos_NewMsgMarkStrEnd = SOCKET_REV_BUF_SIZE - GUID_LEN;
const int   newMsgMaxDataSpace = SOCKET_REV_BUF_SIZE - Pos_msg - GUID_LEN;

typedef  BYTE  ConnMark;
#define  MarkLen        1
#define  ConnMark_Def   0xFF


//接口收到recvBuf为NULL表示断开连接
class IServerRecver
{
public:
	virtual void OnRec(ConnMark mark, UINT32 cmd, char* recvBuf, int BufLen) = 0;
};
class IClientRecver
{
public:
	virtual void OnRec(UINT32 cmd, char* recvBuf, int BufLen) = 0;
};




class MsgCtl
{
	struct Receiver
	{
		Receiver() :m_revSize(0), m_msgSize(0), m_cmd(0) {}
		template<typename RevFinish>
		void rev(BYTE* recvBuf, RevFinish Fun)
		{
			if (isNewMsgMark(recvBuf))
			{
				m_revSize = 0;
				m_msgSize = *((UINT32*)(recvBuf + GUID_LEN));
				m_cmd = *((UINT32*)(recvBuf + Pos_cmd));
				if (m_buf.size() < m_msgSize) m_buf.resize(m_msgSize);

				if (m_msgSize <= newMsgMaxDataSpace)
				{
					memcpy_s(&m_buf[0], m_msgSize, recvBuf + Pos_msg, m_msgSize);
					MsgFinish(Fun);
				}
				return;
			}


			UINT32 CpySize = SOCKET_REV_BUF_SIZE;
			if (m_revSize + SOCKET_REV_BUF_SIZE > m_msgSize)  CpySize = m_msgSize - m_revSize;

			memcpy_s(&m_buf[m_revSize], m_buf.size(), recvBuf, CpySize);
			m_revSize = m_revSize + CpySize;

			if (m_revSize != m_msgSize) return;

			MsgFinish(Fun);
		}
		template<typename RevFinish>
		void MsgFinish(RevFinish Fun)
		{
			Fun(m_cmd, &m_buf[0], m_msgSize);
			m_msgSize = 0;
			m_revSize = 0;
		}
		void Release()
		{
			string().swap(m_buf);
		}
	private:
		bool isNewMsgMark(const BYTE* recvBuf)
		{
			if (!checkGUID(recvBuf, NewMsgMarkStr)) return false;
			if (!checkGUID(recvBuf, NewMsgMarkStrEnd, Pos_NewMsgMarkStrEnd)) return false;
			return true;
		}
		bool checkGUID(const BYTE* recvBuf, const char* guid, int posStart = 0)
		{
			const BYTE* id = (const BYTE*)guid;
			for (int i = 0; i < GUID_LEN; ++i)
			{
				if (id[i] != recvBuf[i + posStart]) return false;
			}
			return true;
		}
		UINT32 m_revSize;
		UINT32 m_msgSize;
		UINT32 m_cmd;
		string m_buf;
	};
	struct Sender
	{
		bool SendMsg(SOCKET sock, UINT32 cmd, const char* msg, UINT32 msgSize)
		{
			char SendBuf[SOCKET_REV_BUF_SIZE] = { 0 };

			memcpy_s(SendBuf, GUID_LEN, NewMsgMarkStr, GUID_LEN);
			memcpy_s(SendBuf + GUID_LEN, NewMsg_Size_LEN, &msgSize, NewMsg_Size_LEN);
			memcpy_s(SendBuf + Pos_cmd, CMD_LEN, &cmd, CMD_LEN);
			memcpy_s(SendBuf + Pos_NewMsgMarkStrEnd, GUID_LEN, NewMsgMarkStrEnd, GUID_LEN);
			if (msgSize <= newMsgMaxDataSpace)
			{
				memcpy_s(SendBuf + Pos_msg, msgSize, msg, msgSize);
				return (SOCKET_REV_BUF_SIZE == send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
			}

			bool suc = (SOCKET_REV_BUF_SIZE == send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
			if (!suc) return false;

			UINT32 hasSendSize = 0;
			while (1)
			{
				if (msgSize <= SOCKET_REV_BUF_SIZE)
				{
					memcpy_s(SendBuf, SOCKET_REV_BUF_SIZE, msg + hasSendSize, msgSize);
					return SOCKET_REV_BUF_SIZE == send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0);
				}


				memcpy_s(SendBuf, SOCKET_REV_BUF_SIZE, msg + hasSendSize, SOCKET_REV_BUF_SIZE);
				suc = (SOCKET_REV_BUF_SIZE == send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
				if (!suc) return false;

				hasSendSize += SOCKET_REV_BUF_SIZE;
				msgSize -= SOCKET_REV_BUF_SIZE;
			}
		}
	};
public:
	template<typename RevFinish>
	void Rev(BYTE* recvBuf, RevFinish Fun)
	{
		m_Receiver.rev(recvBuf, Fun);
	}
	bool Send(SOCKET sock, UINT32 cmd, const char* msg, UINT32 msgSize)
	{
		return m_Sender.SendMsg(sock, cmd, msg, msgSize);
	}
	void Release()
	{
		m_Receiver.Release();
	}
private:
	Receiver m_Receiver;
	Sender m_Sender;
};






class CSocketConn
{
public:
	CSocketConn(SOCKET Conn, IServerRecver* pIServerRecver, IClientRecver* pIClientRecver) :
		m_Conn(Conn), m_bConnect(true), m_ConnMark(ConnMark_Def),
		m_pIServerRecver(pIServerRecver), m_pIClientRecver(pIClientRecver)
	{
		if (pIServerRecver == NULL && pIClientRecver == NULL) return;
		HANDLE h = CreateThread(NULL, 0, RecvThread, this, 0, NULL);
		if (0 != h) CloseHandle(h);
	}
	~CSocketConn()
	{
		Release();
	}
	void Release()
	{
		closesocket(m_Conn);
		m_Conn = NULL;
		m_MsgCtl.Release();
	}
	bool Send(UINT32 cmd, const char* msg, UINT32 msgSize)
	{
		if (!m_bConnect) return false;
		return m_MsgCtl.Send(m_Conn, cmd, msg, msgSize);
	}
	bool ConnectCorrect() { return m_bConnect; }
	BYTE GetMark() { return m_ConnMark; }
private:
	static DWORD WINAPI RecvThread(LPVOID p)
	{
		CSocketConn* pConnect = (CSocketConn*)p;
		pConnect->Recv();
		return 0;
	}
	bool RecvErro(bool bErro)
	{
		if (!bErro) return false;

		m_bConnect = false;

		OnRev(0xFFFFFFFF, NULL, 0);

		return true;
	}
	void OnRev(UINT32 cmd, char* recvBuf, int BufLen)
	{
		if (NULL != m_pIServerRecver) m_pIServerRecver->OnRec(m_ConnMark, cmd, recvBuf, BufLen);
		else m_pIClientRecver->OnRec(cmd, recvBuf, BufLen);
	}
	void Recv()
	{
		char recvBuf[SOCKET_REV_BUF_SIZE] = { 0 };
		int recBufPos = 0;
		int BufLen = SOCKET_ERROR;

		if (m_pIServerRecver != NULL)
		{
			BufLen = recv(m_Conn, recvBuf, 1, 0);
			if (RecvErro(1 != BufLen)) return;
			m_ConnMark = recvBuf[0];
		}

		while (true)
		{

			BufLen = recv(m_Conn, recvBuf + recBufPos, SOCKET_REV_BUF_SIZE - recBufPos, 0);
			recBufPos += BufLen;

			if (RecvErro(SOCKET_ERROR == BufLen)) return;


			if (recBufPos != SOCKET_REV_BUF_SIZE) continue;
			recBufPos = 0;

			m_MsgCtl.Rev((BYTE*)recvBuf, [&](UINT32 cmd, char* buf, int len) {
				OnRev(cmd, buf, len);
				});
		}
	}
private:
	SOCKET m_Conn;
	IServerRecver* m_pIServerRecver;
	IClientRecver* m_pIClientRecver;

	bool m_bConnect;

	MsgCtl m_MsgCtl;

	ConnMark m_ConnMark;
};

class ServerSocket
{
public:
	ServerSocket() :m_pISocketRecver(NULL), m_sockSrv(NULL)
	{
		InitWinSocket();
		list<CSocketConn*>().swap(m_Connect);
		InitializeCriticalSection(&m_cs);
	}
	~ServerSocket()
	{
		for (list<CSocketConn*>::iterator it = m_Connect.begin(); it != m_Connect.end(); ++it)
		{
			(*it)->Release();
			delete* it;
			*it = NULL;
		}
		list<CSocketConn*>().swap(m_Connect);

		closesocket(m_sockSrv);

		DeleteCriticalSection(&m_cs);
	}
public:
	bool Start(int port, IServerRecver* pISocketRecver)
	{
		m_pISocketRecver = pISocketRecver;

		m_sockSrv = socket(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN addrSrv;
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);

		if (SOCKET_ERROR == bind(m_sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))) return false;
		if (SOCKET_ERROR == listen(m_sockSrv, MAX_CONNECT_NUM)) return false;

		HANDLE h = ::CreateThread(NULL, 0, AcceptThread, this, 0, NULL);
		if (0 != h) CloseHandle(h);
		return true;
	}
	bool SendToAllConn(UINT32 cmd, const char* msg, UINT32 msgSize)
	{
		return SendToConn(cmd, msg, msgSize);
	}
	bool SendToMarkConn(BYTE mark, UINT32 cmd, const char* msg, UINT32 msgSize)
	{
		return SendToConn(cmd, msg, msgSize, mark);
	}
private:
	bool SendToConn(UINT32 cmd, const char* msg, UINT32 msgSize, BYTE mark = ConnMark_Def)
	{
		bool sendSuc = false;
		EnterCriticalSection(&m_cs);
		for (list<CSocketConn*>::iterator it = m_Connect.begin(); it != m_Connect.end(); ++it)
		{
			if (!(*it)->ConnectCorrect()) continue;

			if (mark != ConnMark_Def)
				if (mark != (*it)->GetMark()) continue;
			sendSuc = (*it)->Send(cmd, msg, msgSize);
		}
		LeaveCriticalSection(&m_cs);
		return sendSuc;
	}
private:
	static DWORD WINAPI AcceptThread(LPVOID p)
	{
		ServerSocket* pSer = (ServerSocket*)p;
		pSer->OnAccept();
		return 0;
	}
	void OnAccept()
	{
		int len = sizeof(SOCKADDR);
		while (true)
		{
			SOCKADDR_IN addrClient;
			SOCKET sockConn = accept(m_sockSrv, (SOCKADDR*)&addrClient, &len);

			EnterCriticalSection(&m_cs);


			m_Connect.remove_if([](CSocketConn* pSoCo)->bool {
				if (!pSoCo->ConnectCorrect())
				{
					delete pSoCo;
					return true;
				}
				return false;
				});


			if (m_Connect.size() >= MAX_CONNECT_NUM) closesocket(sockConn);
			else m_Connect.push_back(new CSocketConn(sockConn, m_pISocketRecver, NULL));

			LeaveCriticalSection(&m_cs);
		}
	}
private:
	SOCKET m_sockSrv;
	IServerRecver* m_pISocketRecver;

	list<CSocketConn*> m_Connect;
	CRITICAL_SECTION  m_cs;
};



class ClientSocket
{
public:
	ClientSocket() :m_Connect(NULL), m_sockConn(NULL) {
		InitWinSocket();
	}
	~ClientSocket()
	{
		if (NULL == m_Connect) return;
		delete m_Connect;
	}

	bool Start(ConnMark mark, const char* Ip, int port, IClientRecver* Recver)
	{
		m_sockConn = socket(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN addrSrv;

#ifdef __WS2
		inet_pton(AF_INET, Ip, &addrSrv.sin_addr.S_un.S_addr);
#else
		addrSrv.sin_addr.S_un.S_addr = inet_addr(Ip);
#endif   
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);
		if (SOCKET_ERROR == connect(m_sockConn, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))) return false;
		if (MarkLen != send(m_sockConn, (char*)&mark, MarkLen, 0)) return false;

		m_Connect = new CSocketConn(m_sockConn, NULL, Recver);

		return true;
	}
	bool Send(UINT32 cmd, const char* msg, UINT32 msgSize)
	{
		if (NULL == m_Connect) return false;

		if (!m_Connect->ConnectCorrect()) return false;

		return m_Connect->Send(cmd, msg, msgSize);
	}



public:
	char* ProcRev(UINT32& cmdRev, int& lenRev)
	{//IClientRecver 为NULL时使用
		char recvBuf[SOCKET_REV_BUF_SIZE] = { 0 };
		int recBufPos = 0;
		int BufLen = SOCKET_ERROR;
		char* revData = NULL;
		while (true)
		{
			BufLen = recv(m_sockConn, recvBuf + recBufPos, SOCKET_REV_BUF_SIZE - recBufPos, 0);
			recBufPos += BufLen;
			if (SOCKET_ERROR == BufLen) return NULL;
			if (recBufPos != SOCKET_REV_BUF_SIZE) continue;
			recBufPos = 0;
			m_MsgCtl.Rev((BYTE*)recvBuf, [&](UINT32 cmd, char* buf, int len) {
				cmdRev = cmd;
				lenRev = len;
				revData =  buf;
			});
			if (NULL != revData) return revData;
		}
		return NULL;
	}
private:
	CSocketConn* m_Connect;
	SOCKET m_sockConn;
	MsgCtl m_MsgCtl;
};
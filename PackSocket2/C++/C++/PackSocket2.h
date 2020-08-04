#pragma once

struct ClientDataBuf
{
	void Relase()
	{
	}
};



#include <list>
#include <set>
#include <stdint.h>
#include <thread>
#include <atomic>
using namespace std;

#ifdef WIN32
#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")

//#define  __WS2

#ifdef __WS2
#include <WS2tcpip.h>
#endif

inline bool InitSocket() {
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSAStartup(wVersionRequested, &wsaData);
	return true;
}
inline void ClosePackSocket(int sock) { closesocket(sock); }
inline bool ListenPackSocket(int sock, int port, int MAX_CONNECT_NUM)
{
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	if (SOCKET_ERROR == ::bind(sock, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))) return false;
	if (SOCKET_ERROR == listen(sock, MAX_CONNECT_NUM)) return false;
	return true;
}
inline int AcceptPackSocket(int sock)
{
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);
	return accept(sock, (SOCKADDR*)&addrClient, &len);
}
inline bool ConnectPackSocket(int sock, const char* ip, int port)
{
	SOCKADDR_IN addrSrv;
#ifdef __WS2
	inet_pton(AF_INET, ip, &addrSrv.sin_addr.S_un.S_addr);
#else
	addrSrv.sin_addr.S_un.S_addr = inet_addr(ip);
#endif   
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	if (SOCKET_ERROR == connect(sock, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))) return false;
	return true;
}
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
inline void InitSocket() {}
inline void ClosePackSocket(int sock) { close(sock); }
inline bool ListenPackSocket(int sock, int port, int MAX_CONNECT_NUM)
{
	struct sockaddr_in  addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	addrSrv.sin_addr.s_addr = INADDR_ANY;
	bzero(&(addrSrv.sin_zero), 8);
	if (bind(sock, (struct sockaddr *)&addrSrv, sizeof(struct sockaddr)) == -1) return false;
	if (listen(sock, MAX_CONNECT_NUM) == -1) return false;
	return true;
}
inline int  AcceptPackSocket(int sock)
{
	struct sockaddr_in addrClient;
	socklen_t len = sizeof(struct sockaddr_in);
	return accept(sock, (struct sockaddr *)&addrClient, &len);
}
inline bool ConnectPackSocket(int sock, const char* ip, int port)
{
	struct sockaddr_in addrSrv;
	struct hostent *host = gethostbyname(ip);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	addrSrv.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(addrSrv.sin_zero), 8);
	if (connect(sock, (struct sockaddr *)&addrSrv, sizeof(struct sockaddr)) == -1) return false;
	return true;
}
#endif




struct User
{
	uint8_t   answerMode;
	uint8_t   id;
	char   name[32];
	char   password[32];
};
const int UserLen = sizeof(User);


const int headMsgLen = 1012;
struct MsgHead
{
	uint32_t cmd;
	uint32_t code;
	uint32_t BigMsgLen;
	char     headMsg[headMsgLen];
};
const int MsgHeadLen = sizeof(MsgHead);

const int headMsgDataLen = headMsgLen - 4;
struct HeadMsgData
{
	uint32_t len;
	char     data[headMsgDataLen];
};
static_assert(sizeof(HeadMsgData) == headMsgLen, "HeadMsgLenErr");


class PackSocketMSG
{
public:
	MsgHead head;
	const void *data;

	void InitCodeMsg(int cmd = 0, int code = 0)
	{
		head.cmd = cmd;
		head.code = code;
		head.BigMsgLen = 0;
		data = 0;
	}
	void InitHeadMsg(int cmd, void *msg, int msgLen)
	{
		InitCodeMsg(cmd);
		memcpy(head.headMsg, msg, msgLen);
	}
	void InitBigMsg(int cmd, int BigMsgLen, void *msgdata, int code = 0)
	{
		InitCodeMsg(cmd, code);
		head.BigMsgLen = BigMsgLen;
		data = msgdata;
	}

public:
	void SetData(const void* dat, int len)
	{
		if (0 == len) return;
		if (len <= headMsgDataLen)
		{
			HeadMsgData *pdata = (HeadMsgData *)head.headMsg;
			pdata->len = len;
			memcpy(pdata->data, dat, len);
		}
		else
		{
			head.BigMsgLen = len;
			data = dat;
		}
	}
	const void* GetData(int* retLen)
	{
		if (head.BigMsgLen == 0)
		{
			HeadMsgData *pdata = (HeadMsgData *)head.headMsg;
			*retLen = pdata->len;
			return pdata->data;
		}
		else
		{
			*retLen = head.BigMsgLen;
			return data;
		}
	}
};

enum PS2ErrDEF
{
	PS2ErrDEF_UserCheckSuc = 0,
	PS2ErrDEF_UserCheckErr,

	PS2ErrDEF_SocketSendErr,
	PS2ErrDEF_SocketRevErr,
};


class IServerRecver
{
public:
	virtual void OnRec(User *user, PackSocketMSG *msg, ClientDataBuf &clbuf) = 0;
	virtual bool UserCheck(User *user) = 0;
	virtual void OnDisConnected(User *user, uint8_t errCode) = 0;
};
class IClientRecver
{
public:
	virtual void OnRec(PackSocketMSG *msg) = 0;
	virtual void OnDisConnected(uint8_t errCode) = 0;
};



class CSocketConn
{
	int m_Conn;
	IServerRecver *m_pIServerRecver;
	IClientRecver *m_pIClientRecver;
	bool m_bConnect;

	User m_user;
	string m_revbuf;

	ClientDataBuf m_clbuf;

	PackSocketMSG m_MsgRevData;
public:
	CSocketConn()
	{
		m_Conn = 0;
		m_bConnect = false;
	}
	void Release()
	{
		m_bConnect = false;
		ClosePackSocket(m_Conn);
		m_Conn = 0;
		string().swap(m_revbuf);
		m_clbuf.Relase();
	}
	bool InitByServer(int Conn, IServerRecver *pIServerRecver)
	{
		m_Conn = Conn;
		m_pIServerRecver = pIServerRecver;
		m_pIClientRecver = 0;
		m_bConnect = true;

		if (!FetchData()) return false;
		User *user = (User*)m_MsgRevData.head.headMsg;
		if (0 != user && m_pIServerRecver->UserCheck(user))
		{
			m_MsgRevData.InitCodeMsg(PS2ErrDEF_UserCheckSuc);
			if (!Send(&m_MsgRevData)) return false;
			m_user = *user;
			RecvThread();
			return true;
		}
		else
		{
			m_MsgRevData.InitCodeMsg(PS2ErrDEF_UserCheckErr);
			Send(&m_MsgRevData);
			return false;
		}
	}
	bool InitByClient(int Conn, IClientRecver *pIClientRecver, User *_user)
	{
		m_Conn = Conn;
		m_pIServerRecver = 0;
		m_pIClientRecver = pIClientRecver;
		m_bConnect = true;
		m_user = *_user;

		m_MsgRevData.InitHeadMsg(0, _user, UserLen);
		if (0 == Send_Answer(&m_MsgRevData)) return false;
		if (m_MsgRevData.head.cmd != PS2ErrDEF_UserCheckSuc)
		{
			OnDisConnected(PS2ErrDEF_UserCheckErr);
			return false;
		}

		if (!m_user.answerMode) RecvThread();
		return true;
	}
	bool ConnectCorrect() { return m_bConnect; }
	User* GetUser() { return &m_user; }
	bool Send(PackSocketMSG *msg)
	{
		if (!m_bConnect) return false;

		bool ret = SendData(&msg->head, MsgHeadLen);
		if (msg->head.BigMsgLen <= 0) return ret;

		if (!ret) return false;
		return SendData(msg->data, msg->head.BigMsgLen);
	}


	PackSocketMSG* Send_Answer(PackSocketMSG *msg)
	{
		if (!Send(msg)) return 0;
		if (!FetchData()) return 0;
		return &m_MsgRevData;
	}
private:
	bool SendData(const void *data, int dataLen)
	{
		char *msg = (char*)data;
		int hasSendSize = 0;
		while (1)
		{
			int sendLen = send(m_Conn, msg + hasSendSize, dataLen - hasSendSize, 0);
			if (sendLen <= 0)
			{
				OnDisConnected(PS2ErrDEF_SocketSendErr);
				return false;
			}
			hasSendSize += sendLen;
			if (hasSendSize == dataLen) return true;
		}
	}
	void RecvThread()
	{
		thread* pThr = new thread(RecvThreadRun, this);
		pThr->detach();
		delete  pThr;
	}
	static void  RecvThreadRun(CSocketConn *pConnect)
	{
		while (true)
		{
			PackSocketMSG* msg = pConnect->FetchData();
			if (0 == msg) return;
			pConnect->OnRev(msg);
		}
	}
	PackSocketMSG* FetchData()
	{
		MsgHead *phead = (MsgHead*)RevData(MsgHeadLen);
		if (NULL == phead) return 0;
		m_MsgRevData.head = *phead;
		m_MsgRevData.data = RevData(m_MsgRevData.head.BigMsgLen);
		return &m_MsgRevData;
	}
	void* RevData(int revTotoalLen)
	{
		if (revTotoalLen <= 0)
		{
			if (m_revbuf.capacity() < 10) m_revbuf.resize(10);
			return  &m_revbuf[0];
		}

		if (m_revbuf.capacity() < revTotoalLen) m_revbuf.resize(revTotoalLen);
		char *BUF = &m_revbuf[0];
		int recBufPos = 0;
		while (true)
		{
			int revlen = recv(m_Conn, BUF + recBufPos, revTotoalLen - recBufPos, 0);
			recBufPos += revlen;
			if (revlen <= 0)
			{
				OnDisConnected(PS2ErrDEF_SocketRevErr);
				return NULL;
			}
			if (recBufPos == revTotoalLen) return BUF;
		}
	}
	void OnRev(PackSocketMSG* msg)
	{
		if (NULL != m_pIServerRecver)
		{
			m_pIServerRecver->OnRec(&m_user, msg, m_clbuf);
			if (m_user.answerMode) Send(msg);
		}
		else m_pIClientRecver->OnRec(msg);
	}
	void OnDisConnected(uint8_t errCode)
	{
		m_bConnect = false;
		if (NULL != m_pIServerRecver) m_pIServerRecver->OnDisConnected(&m_user, errCode);
		else m_pIClientRecver->OnDisConnected(errCode);
		Release();
	}
};



class ServerSocket
{
	int m_MAX_CONNECT_NUM;
public:
	ServerSocket()
	{
		m_pISocketRecver = 0;
		m_sockSrv = 0;
		list<CSocketConn*>().swap(m_Connect);
		InitSocket();
	}
	~ServerSocket()
	{
		for (list<CSocketConn*>::iterator it = m_Connect.begin(); it != m_Connect.end(); ++it)
		{
			(*it)->Release();
			delete *it;
		}
		list<CSocketConn*>().swap(m_Connect);
		ClosePackSocket(m_sockSrv);
	}
public:
	bool Start(int port, IServerRecver *pISocketRecver, int MAX_CONNECT_NUM)
	{
		m_MAX_CONNECT_NUM = MAX_CONNECT_NUM;
		m_pISocketRecver = pISocketRecver;

		m_sockSrv = socket(AF_INET, SOCK_STREAM, 0);

		if (!ListenPackSocket(m_sockSrv, port, m_MAX_CONNECT_NUM)) return false;

		AcceptThread();
		return true;
	}
	bool SendToAllConn(PackSocketMSG *msg)
	{
		return SendToConn(msg);
	}
	bool SendToMarkConn(uint8_t id, PackSocketMSG *msg)
	{
		return SendToConn(msg, id);
	}
private:
	bool SendToConn(PackSocketMSG *msg, uint8_t id = 0xFF)
	{
		bool sendSuc = false;
		while (lock.test_and_set());
		for (list<CSocketConn*>::iterator it = m_Connect.begin(); it != m_Connect.end(); ++it)
		{
			if (!(*it)->ConnectCorrect()) continue;

			if (id != 0xFF)
				if (id != (*it)->GetUser()->id) continue;
			sendSuc = (*it)->Send(msg);
		}
		lock.clear();
		return sendSuc;
	}
private:
	void AcceptThread()
	{
		thread* pThr = new thread(AcceptThreadRun, this);
		pThr->detach();
		delete  pThr;
	}
	static void AcceptThreadRun(ServerSocket *pSer)
	{
		pSer->OnAccept();
	}

	void OnAccept()
	{
		while (true)
		{
			int sockConn = AcceptPackSocket(m_sockSrv);
			while (lock.test_and_set());
			m_Connect.remove_if([](CSocketConn* pSo)->bool {
				if (!pSo->ConnectCorrect())
				{
					delete pSo;
					return true;
				}
				return false;
				});


			if (m_Connect.size() >= m_MAX_CONNECT_NUM)
			{
				ClosePackSocket(sockConn);
			}
			else
			{
				CSocketConn *pSo = new CSocketConn();
				if (pSo->InitByServer(sockConn, m_pISocketRecver)) m_Connect.push_back(pSo);
				else
				{
					pSo->Release();
					delete pSo;
				}
			}

			lock.clear();
		}
	}
private:
	int m_sockSrv;
	IServerRecver *m_pISocketRecver;

	list<CSocketConn*> m_Connect;

	atomic_flag lock = ATOMIC_FLAG_INIT;
};



class ClientSocket
{
public:
	ClientSocket() {
		InitSocket();
	}

	bool Start(User *user, const char *ip, int port, IClientRecver *Recver)
	{
		if (m_Connect.ConnectCorrect()) return true;

		int sockConn = socket(AF_INET, SOCK_STREAM, 0);

		if (!ConnectPackSocket(sockConn, ip, port)) return false;

		if (!m_Connect.InitByClient(sockConn, Recver, user)) return false;

		return true;
	}
	bool Send(PackSocketMSG *msg)
	{
		if (!m_Connect.ConnectCorrect()) return false;
		return m_Connect.Send(msg);
	}
	PackSocketMSG* Send_Answer(PackSocketMSG *msg)
	{
		if (!m_Connect.ConnectCorrect()) return NULL;
		return m_Connect.Send_Answer(msg);
	}
private:
	CSocketConn m_Connect;
};
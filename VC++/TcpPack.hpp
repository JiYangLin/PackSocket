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
inline bool InitWinSocket(){
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSAStartup( wVersionRequested, &wsaData );
	return true;
}




#define  MAX_CONNECT_NUM  20
#define  SOCKET_REV_BUF_SIZE     1024   //固定每个数据长度

#define  GUID_LEN    32
#define  NewMsgMarkStr    "6DB62435625348A28426C876F6B04A88"
#define  NewMsgMarkStrEnd "CE1D1E7C9DBF4951BB414B6D9D2AA1C1"
#define  NewMsg_Size_LEN  4
typedef  UINT32 NewMsgSizeType;

typedef  BYTE  ConnMark;    
#define  MarkLen        1
#define  ConnMark_Def   0xFF


//接口收到recvBuf为NULL表示断开连接
class IServerRecver
{
public:
	virtual void OnRec(ConnMark mark,char *recvBuf, int BufLen)=0;
};
class IClientRecver
{
public:
	virtual void OnRec(char *recvBuf, int BufLen)=0;
};




class MsgCtl
{
	struct Receiver
	{
		Receiver():m_revSize(0),m_msgSize(0){}
		template<typename RevFinish>
		void rev(BYTE *recvBuf,RevFinish Fun)
		{
			if (isNewMsgMark(recvBuf))
			{	
				m_revSize = 0;
				m_msgSize = *((NewMsgSizeType*)(recvBuf + GUID_LEN));	
				if (m_buf.size() < m_msgSize) m_buf.resize(m_msgSize);
				return ;
			}
	

			int CpySize = SOCKET_REV_BUF_SIZE;
			if (m_revSize + SOCKET_REV_BUF_SIZE > m_msgSize)  CpySize = m_msgSize - m_revSize;


			if (0 == m_msgSize) return;
			if (m_buf.size() < m_revSize + CpySize) return;


			memcpy_s(&m_buf[m_revSize],m_buf.size(),recvBuf,CpySize);
			m_revSize = m_revSize + CpySize;


			if(m_revSize != m_msgSize) return;

			Fun(&m_buf[0],m_revSize);
			m_msgSize = 0;
			m_revSize = 0;
		}
		void Release()
		{
			string().swap(m_buf);
		}
	private:
		bool isNewMsgMark(const BYTE *recvBuf)
		{
			if (!checkGUID(recvBuf,NewMsgMarkStr)) return false;	
			int EndGDIDPos = SOCKET_REV_BUF_SIZE - GUID_LEN;
			if(!checkGUID(recvBuf,NewMsgMarkStrEnd,EndGDIDPos)) return false;
			for(int i = GUID_LEN + NewMsg_Size_LEN; i < EndGDIDPos; ++i)
				if (0 != recvBuf[i]) return false;
			return true;
		}
		bool checkGUID(const BYTE *recvBuf,const char *guid,int posStart = 0)
		{
			const BYTE * id = (const BYTE *)guid;
			for (int i = 0 ; i < GUID_LEN; ++i)
			{
				if (id[i] != recvBuf[i+posStart]) return false;
			}
			return true;
		}
		UINT32 m_revSize;
		UINT32 m_msgSize;
		string m_buf;
	};
	struct Sender
	{
		bool SendMsg(SOCKET sock,const char * msg,NewMsgSizeType msgSize)
		{
			char SendBuf[SOCKET_REV_BUF_SIZE] = {0};

			memcpy_s(SendBuf,GUID_LEN,NewMsgMarkStr,GUID_LEN);
			memcpy_s(SendBuf + GUID_LEN,NewMsg_Size_LEN,&msgSize,NewMsg_Size_LEN);
			memcpy_s(SendBuf + SOCKET_REV_BUF_SIZE - GUID_LEN,GUID_LEN,NewMsgMarkStrEnd,GUID_LEN);
			bool suc = (SOCKET_REV_BUF_SIZE ==  send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
			if(!suc) return false;


			UINT32 hasSendSize = 0;
			while(1)
			{
				memset(SendBuf,0,SOCKET_REV_BUF_SIZE);
				if (msgSize <= SOCKET_REV_BUF_SIZE)
				{
					memcpy_s(SendBuf,SOCKET_REV_BUF_SIZE,msg + hasSendSize,msgSize);

					return SOCKET_REV_BUF_SIZE ==  send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0);
				}


				memcpy_s(SendBuf,SOCKET_REV_BUF_SIZE,msg + hasSendSize,SOCKET_REV_BUF_SIZE);

				suc = (SOCKET_REV_BUF_SIZE ==  send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
				if(!suc) return false;


				hasSendSize += SOCKET_REV_BUF_SIZE;
				msgSize -= SOCKET_REV_BUF_SIZE;
			}
		}
	};
public:
	template<typename RevFinish>
	void Rev(BYTE *recvBuf,RevFinish Fun)
	{
		m_Receiver.rev(recvBuf,Fun);
	}
	bool Send(SOCKET sock,const char * msg,NewMsgSizeType msgSize)
	{
		return m_Sender.SendMsg(sock,msg,msgSize);
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
	CSocketConn(SOCKET Conn,IServerRecver *pIServerRecver,IClientRecver *pIClientRecver):m_Conn(Conn), m_bConnect(true),m_ConnMark(ConnMark_Def)
	{
		m_pIServerRecver = pIServerRecver;
		m_pIClientRecver = pIClientRecver;
		m_Thread = CreateThread(NULL,0,RecvThread,this,0,NULL);
	}
	~CSocketConn()
	{
		Release();
	}
	void Release()
	{
		TerminateThread(m_Thread,0);
		m_Thread = NULL;
		closesocket(m_Conn);
		m_Conn = NULL;
		m_MsgCtl.Release();
	}
	bool Send(const char * msg,NewMsgSizeType msgSize)
	{
		if (!m_bConnect) return false;
		return m_MsgCtl.Send(m_Conn,msg,msgSize);
	}
	bool ConnectCorrect() { return m_bConnect; }
	BYTE GetMark(){return m_ConnMark; }
private:
	static DWORD WINAPI RecvThread(LPVOID p)
	{
		CSocketConn *pConnect = (CSocketConn *)p;
		pConnect->Recv();
		return 0;
	}
	bool RecvErro(bool bErro)
	{
		if (!bErro) return false;
		
		m_bConnect = false;

	    OnRev(NULL,0);

		return true;
	}
	void OnRev(char *recvBuf, int BufLen)
	{
		if (NULL != m_pIServerRecver) m_pIServerRecver->OnRec(m_ConnMark,recvBuf,BufLen);
		else m_pIClientRecver->OnRec(recvBuf,BufLen);
	}
	void Recv()
	{
		char recvBuf[SOCKET_REV_BUF_SIZE] = {0};
		int recBufPos = 0;
		int BufLen = SOCKET_ERROR;
		
		if(m_pIServerRecver != NULL)
		{
			BufLen = recv(m_Conn,recvBuf,1,0);
			if(RecvErro(1 != BufLen)) return;
			m_ConnMark = recvBuf[0];
		}

		while(true)
		{
			
			BufLen = recv(m_Conn,recvBuf + recBufPos,SOCKET_REV_BUF_SIZE - recBufPos,0);
			recBufPos += BufLen;

			if(RecvErro(SOCKET_ERROR == BufLen)) return;

		
			if (recBufPos != SOCKET_REV_BUF_SIZE) continue;
			recBufPos = 0;

			m_MsgCtl.Rev((BYTE*)recvBuf,[&](char *buf,int len){
				OnRev(buf,len);
			});
		}
	}
private:
	SOCKET m_Conn;
	IServerRecver *m_pIServerRecver;
	IClientRecver *m_pIClientRecver;
	HANDLE m_Thread;

	bool m_bConnect;

	MsgCtl m_MsgCtl;

	ConnMark m_ConnMark;
};

class ServerSocket
{
public:
	ServerSocket():m_pISocketRecver(NULL),m_sockSrv(NULL),m_AcceptThrHand(NULL)
	{
		list<CSocketConn*>().swap(m_Connect);
		InitializeCriticalSection(&m_cs);
	}
	~ServerSocket()
	{
		TerminateThread(m_AcceptThrHand,0);
		CloseHandle(m_AcceptThrHand);

		for (list<CSocketConn*>::iterator it = m_Connect.begin();it != m_Connect.end(); ++it)
		{
			(*it)->Release();
			delete *it;
			*it = NULL;
		}
		list<CSocketConn*>().swap(m_Connect);

		closesocket(m_sockSrv);

		DeleteCriticalSection(&m_cs);
	}
public:
	bool Start(int port,IServerRecver *pISocketRecver)
	{
		m_pISocketRecver = pISocketRecver;

		m_sockSrv = socket(AF_INET,SOCK_STREAM,0);
		SOCKADDR_IN addrSrv;
		addrSrv.sin_addr.S_un.S_addr=htonl(INADDR_ANY);
		addrSrv.sin_family=AF_INET;
		addrSrv.sin_port=htons(port); 

		if(SOCKET_ERROR == bind(m_sockSrv,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR))) return false;
		if(SOCKET_ERROR == listen(m_sockSrv,MAX_CONNECT_NUM)) return false;

		m_AcceptThrHand = ::CreateThread(NULL,0,AcceptThread,this,0,NULL);

		return true;
	}
	bool SendToAllConn(const char * msg,NewMsgSizeType msgSize)
	{
		return SendToConn(msg,msgSize);
	}
	bool SendToMarkConn( BYTE mark,const char * msg,NewMsgSizeType msgSize)
	{
		return SendToConn(msg,msgSize,mark);
	}
private:
	bool SendToConn(const char * msg,NewMsgSizeType msgSize,BYTE mark = ConnMark_Def)
	{
		bool sendSuc = false;
		EnterCriticalSection(&m_cs);
		for (list<CSocketConn*>::iterator it = m_Connect.begin();it != m_Connect.end(); ++it)
		{
			if (!(*it)->ConnectCorrect()) continue;
		
			if(mark != ConnMark_Def)
				if(mark != (*it)->GetMark()) continue;	
			sendSuc = (*it)->Send(msg, msgSize);
		}
		LeaveCriticalSection(&m_cs);
		return sendSuc;
	}
private:
	static DWORD WINAPI AcceptThread(LPVOID p)
	{	
		ServerSocket *pSer = (ServerSocket *)p;
		pSer->OnAccept();
		return 0;
	}
	void OnAccept()
	{
		int len = sizeof(SOCKADDR);	
		while(true)
		{
			SOCKADDR_IN addrClient;
			SOCKET sockConn = accept(m_sockSrv,(SOCKADDR*)&addrClient,&len);

			EnterCriticalSection(&m_cs);

		
			m_Connect.remove_if([](CSocketConn* pSoCo)->bool {
				if (!pSoCo->ConnectCorrect())
				{
					delete pSoCo;
					return true;
				}
				return false;
			});

			m_Connect.push_back(new CSocketConn(sockConn, m_pISocketRecver,NULL));
			if (m_Connect.size() >= MAX_CONNECT_NUM) return;
			LeaveCriticalSection(&m_cs);			
		}
	}
private:
	SOCKET m_sockSrv;
	IServerRecver *m_pISocketRecver;

	HANDLE m_AcceptThrHand;
	list<CSocketConn*> m_Connect;
	CRITICAL_SECTION  m_cs;
};



class ClientSocket
{
public:
	ClientSocket() :m_Connect(NULL){}
	~ClientSocket()
	{
		if (NULL == m_Connect) return;
		delete m_Connect;
	}

	bool Start(ConnMark mark,const char *Ip,int port,IClientRecver *Recver)
	{

		SOCKET sockConn = socket(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN addrSrv;

#ifdef __WS2
		inet_pton(AF_INET, Ip, &addrSrv.sin_addr.S_un.S_addr);
#else
		addrSrv.sin_addr.S_un.S_addr = inet_addr(Ip);
#endif   
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);
		if (SOCKET_ERROR == connect(sockConn, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))) return false;
		if (MarkLen != send(sockConn, (char*)&mark, MarkLen, 0)) return false;

		m_Connect = new CSocketConn(sockConn, NULL,Recver);

		return true;
	}
	bool Send(const char * msg,NewMsgSizeType msgSize)
	{
		if (NULL == m_Connect) return false;

		if (!m_Connect->ConnectCorrect()) return false;

		return m_Connect->Send(msg,msgSize);
	}
private:
	CSocketConn* m_Connect;
};
#ifndef TCPPACK_H
#define TCPPACK_H

#include <list>
#include <string.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
using namespace std;




#define  MAX_CONNECT_NUM  20
#define  SOCKET_REV_BUF_SIZE     1024   

#define  GUID_LEN    16
#define  NewMsgMarkStr    "4A6959616E674C696E6DB62435625348"
#define  NewMsgMarkStrEnd "414B6D9D2AA1C14A6959616E674C696E"
#define  NewMsg_Size_LEN  4
#define  CMD_LEN    4

const int Pos_cmd = 20;
const int Pos_msg = 24;
const int Pos_NewMsgMarkStrEnd = 1008;
const int newMsgMaxDataSpace = 984;

typedef  u_char BYTE;
typedef  BYTE  ConnMark;
#define  MarkLen        1
#define  ConnMark_Def   0xFF


class IServerRecver
{
public:
	virtual void OnRec(ConnMark mark,u_int32_t cmd,char *recvBuf, int BufLen)=0;
};
class IClientRecver
{
public:
	virtual void OnRec(u_int32_t cmd,char *recvBuf, int BufLen)=0;
};



class  IRevFinish
{
public:
    virtual void OnRevFinish(u_int32_t cmd,char *recvBuf,u_int32_t BufLen)=0;
};
class MsgCtl
{
    struct Receiver
    {
        Receiver():m_revSize(0),m_msgSize(0),m_cmd(0){}
        void rev(BYTE *recvBuf,IRevFinish* Fun)
        {
			
			if (isNewMsgMark(recvBuf))
			{	
				m_revSize = 0;
				m_msgSize = *((u_int32_t*)(recvBuf + GUID_LEN));
				m_cmd = *((u_int32_t*)(recvBuf + Pos_cmd));
				if (m_buf.size() < m_msgSize) m_buf.resize(m_msgSize);

				if (m_msgSize <= newMsgMaxDataSpace)
				{
					memcpy(&m_buf[0], recvBuf + Pos_msg, m_msgSize);
					MsgFinish(Fun);
				}
				return ;
			}
			
	
            int CpySize = SOCKET_REV_BUF_SIZE;
            if (m_revSize + SOCKET_REV_BUF_SIZE > m_msgSize)  CpySize = m_msgSize - m_revSize;

            memcpy(&m_buf[m_revSize],recvBuf,CpySize);
            m_revSize = m_revSize + CpySize;

            if(m_revSize != m_msgSize) return;

            MsgFinish(Fun);
        }
		void MsgFinish(IRevFinish* Fun)
		{
			Fun->OnRevFinish(m_cmd,&m_buf[0],m_msgSize);
			m_msgSize = 0;
			m_revSize = 0;
		}
        void Release()
        {
            m_buf.clear();
        }
    private:
        bool isNewMsgMark(const BYTE *recvBuf)
        {
            if (!checkGUID(recvBuf,NewMsgMarkStr)) return false;
            if(!checkGUID(recvBuf,NewMsgMarkStrEnd,Pos_NewMsgMarkStrEnd)) return false;
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
        u_int32_t m_revSize;
        u_int32_t m_msgSize;
		u_int32_t m_cmd;
        string m_buf;
    };
    struct Sender
    {
        bool SendMsg(int sock,u_int32_t cmd,const char * msg,u_int32_t msgSize)
        {
			char SendBuf[SOCKET_REV_BUF_SIZE] = {0};

			memcpy(SendBuf,NewMsgMarkStr,GUID_LEN);
			memcpy(SendBuf + GUID_LEN,&msgSize,NewMsg_Size_LEN);
			memcpy(SendBuf + Pos_cmd,  &cmd, CMD_LEN);
			memcpy(SendBuf + Pos_NewMsgMarkStrEnd,NewMsgMarkStrEnd,GUID_LEN);
			if (msgSize <= newMsgMaxDataSpace)
			{
				memcpy(SendBuf + Pos_msg, msg, msgSize);
				return (SOCKET_REV_BUF_SIZE == send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
			}
			bool suc = (SOCKET_REV_BUF_SIZE ==  send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
			if(!suc) return false;

            u_int32_t hasSendSize = 0;
			while(1)
			{
				if (msgSize <= SOCKET_REV_BUF_SIZE)
				{
					memcpy(SendBuf,msg + hasSendSize,msgSize);
					return SOCKET_REV_BUF_SIZE ==  send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0);
				}
			

				memcpy(SendBuf,msg + hasSendSize,SOCKET_REV_BUF_SIZE);
				suc = (SOCKET_REV_BUF_SIZE ==  send(sock, SendBuf, SOCKET_REV_BUF_SIZE, 0));
				if(!suc) return false;

				hasSendSize += SOCKET_REV_BUF_SIZE;
				msgSize -= SOCKET_REV_BUF_SIZE;
			}

        }
    };
public:
    void Rev(BYTE *recvBuf,IRevFinish* Fun)
    {
        m_Receiver.rev(recvBuf,Fun);
    }
    bool Send(int sock,u_int32_t cmd,const char * msg,u_int32_t msgSize)
    {
        return m_Sender.SendMsg(sock,cmd,msg,msgSize);
    }
    void Release()
    {
        m_Receiver.Release();
    }
private:
    Receiver m_Receiver;
    Sender m_Sender;
};






class CSocketConn:public IRevFinish
{
public:
    CSocketConn(int Conn,IServerRecver *pIServerRecver,IClientRecver *pIClientRecver):m_Conn(Conn), m_bConnect(true),m_ConnMark(ConnMark_Def)
    {
        m_pIServerRecver = pIServerRecver;
        m_pIClientRecver = pIClientRecver;
        if (pIServerRecver == 0 && pIClientRecver == 0) return;
        pthread_create(&m_Thread,0,RecvThread,this);
    }
    virtual ~CSocketConn()
    {
        Release();
    }
    void Release()
    {
        pthread_cancel(m_Thread);
        m_Thread = 0;
        close(m_Conn);
        m_Conn = 0;
        m_MsgCtl.Release();
    }
    bool Send(u_int32_t cmd,const char * msg,u_int32_t msgSize)
    {
        if (!m_bConnect) return false;
        return m_MsgCtl.Send(m_Conn,cmd,msg,msgSize);
    }
    bool ConnectCorrect() { return m_bConnect; }
    BYTE GetMark(){return m_ConnMark; }
private:
    static void*  RecvThread(void* p)
    {
        CSocketConn *pConnect = (CSocketConn *)p;
        pConnect->Recv();
        return 0;
    }
    bool RecvErro(bool bErro)
    {
        if (!bErro) return false;

        m_bConnect = false;

        OnRevFinish(0xFFFFFFFF,NULL,0);

        return true;
    }
    virtual void OnRevFinish(u_int32_t cmd,char *recvBuf,u_int32_t BufLen)
    {
        if (NULL != m_pIServerRecver) m_pIServerRecver->OnRec(m_ConnMark,cmd,recvBuf,BufLen);
        else m_pIClientRecver->OnRec(cmd,recvBuf,BufLen);
    }
    void Recv()
    {
        char recvBuf[SOCKET_REV_BUF_SIZE] = {0};
        int recBufPos = 0;
        int BufLen = 0;

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

            if(RecvErro(BufLen <= 0)) return;


            if (recBufPos != SOCKET_REV_BUF_SIZE) continue;
            recBufPos = 0;

            m_MsgCtl.Rev((BYTE*)recvBuf,this);
        }
    }
private:
    int            m_Conn;
    IServerRecver *m_pIServerRecver;
    IClientRecver *m_pIClientRecver;
    pthread_t      m_Thread;

    bool m_bConnect;

    MsgCtl m_MsgCtl;

    ConnMark m_ConnMark;
};

class ServerSocket
{
public:
    ServerSocket():m_pISocketRecver(NULL)
    {
        pthread_mutex_init(&m_lock,0);
    }
    ~ServerSocket()
    {
        pthread_cancel(m_AcceptThrHand);
        pthread_mutex_destroy(&m_lock);

        for (list<CSocketConn*>::iterator it = m_Connect.begin();it != m_Connect.end(); ++it)
        {
            (*it)->Release();
            delete *it;
            *it = NULL;
        }
        close(m_sockSrv);

        pthread_mutex_destroy(&m_lock);
    }
public:
    bool Start(int port,IServerRecver *pISocketRecver)
    {
        m_pISocketRecver = pISocketRecver;

        m_sockSrv = socket(AF_INET,SOCK_STREAM,0);
        struct sockaddr_in  addrSrv;
        addrSrv.sin_family=AF_INET;
        addrSrv.sin_port=htons(port);
        addrSrv.sin_addr.s_addr = INADDR_ANY;
        bzero(&(addrSrv.sin_zero),8);

        if (bind(m_sockSrv, (struct sockaddr *)&addrSrv, sizeof(struct sockaddr)) == -1)
        {
            cout<<"bind erro"<<endl;
            return false;
        }
        if (listen(m_sockSrv, MAX_CONNECT_NUM) == -1)
        {
            cout<<"listen erro"<<endl;
            return false;
        }

        pthread_create(&m_AcceptThrHand,0,AcceptThread,this);

        return true;
    }
    bool SendToAllConn(u_int32_t cmd,const char * msg,u_int32_t msgSize)
    {
        return SendToConn(cmd,msg,msgSize);
    }
    bool SendToMarkConn( BYTE mark,u_int32_t cmd,const char * msg,u_int32_t msgSize)
    {
        return SendToConn(cmd,msg,msgSize,mark);
    }
private:
    bool SendToConn(u_int32_t cmd,const char * msg,u_int32_t msgSize,BYTE mark = ConnMark_Def)
    {
        bool sendSuc = false;
        pthread_mutex_lock(&m_lock);
        for (list<CSocketConn*>::iterator it = m_Connect.begin();it != m_Connect.end(); ++it)
        {
            if (!(*it)->ConnectCorrect()) continue;

            if(mark != ConnMark_Def)
                if(mark != (*it)->GetMark()) continue;
            sendSuc = (*it)->Send(cmd,msg, msgSize);
        }
        pthread_mutex_unlock(&m_lock);
        return sendSuc;
    }
private:
    static void* AcceptThread(void* p)
    {
        ServerSocket *pSer = (ServerSocket *)p;
        pSer->OnAccept();
        return 0;
    }
    static bool needRemove(CSocketConn* pSoCo)
    {
        if (!pSoCo->ConnectCorrect())
        {
            delete pSoCo;
            return true;
        }
        return false;
    }
    void OnAccept()
    {
        while(true)
        {
            struct sockaddr_in addrClient;
            socklen_t len = sizeof(struct sockaddr_in);
            int sockConn = accept(m_sockSrv, (struct sockaddr *)&addrClient, &len);

            pthread_mutex_lock(&m_lock);

            m_Connect.remove_if(needRemove);

			if (m_Connect.size() >= MAX_CONNECT_NUM) close(sockConn);
			else m_Connect.push_back(new CSocketConn(sockConn, m_pISocketRecver, NULL));
			
            pthread_mutex_unlock(&m_lock);
        }
    }
private:
    int            m_sockSrv;
    IServerRecver *m_pISocketRecver;

    pthread_t     m_AcceptThrHand;
    list<CSocketConn*> m_Connect;
    pthread_mutex_t  m_lock;
};



class ClientSocket:public IRevFinish
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
        m_sockConn = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addrSrv;


        struct hostent *host = gethostbyname(Ip);
        addrSrv.sin_family=AF_INET;
        addrSrv.sin_port=htons(port);
        addrSrv.sin_addr = *((struct in_addr *)host->h_addr);
        bzero(&(addrSrv.sin_zero),8);

        if (connect(m_sockConn, (struct sockaddr *)&addrSrv, sizeof(struct sockaddr)) == -1) return false;

        if (MarkLen != send(m_sockConn, (char*)&mark, MarkLen, 0)) return false;

        m_Connect = new CSocketConn(m_sockConn, NULL,Recver);

        return true;
    }
    bool Send(u_int32_t cmd,const char * msg,u_int32_t msgSize)
    {
        if (NULL == m_Connect) return false;

        if (!m_Connect->ConnectCorrect()) return false;

        return m_Connect->Send(cmd,msg,msgSize);
    }

public:
	char* ProcRev(u_int32_t& cmdRev, int& lenRev)
	{
		char recvBuf[SOCKET_REV_BUF_SIZE] = { 0 };
		int recBufPos = 0;
		int BufLen = 0;
		m_revData = 0;
		while (true)
		{
			BufLen = recv(m_sockConn, recvBuf + recBufPos, SOCKET_REV_BUF_SIZE - recBufPos, 0);
			recBufPos += BufLen;
			if(BufLen <= 0) return 0;
			if (recBufPos != SOCKET_REV_BUF_SIZE) continue;
			recBufPos = 0;
			m_MsgCtl.Rev((BYTE*)recvBuf, this);
			if (0 != m_revData)
            {
                cmdRev = m_cmdRev;
                lenRev = m_lenRev;
                return m_revData;
            }
		}
		return 0;
	}
private:
    char* m_revData;
    int m_cmdRev;
    int m_lenRev;
    virtual void OnRevFinish(u_int32_t cmd,char *recvBuf,u_int32_t BufLen)
    {
	    m_cmdRev = cmd;
		m_lenRev = BufLen;
		m_revData =  recvBuf;
    }
private:
    CSocketConn* m_Connect;
    int m_sockConn;
	MsgCtl m_MsgCtl;
};
#endif

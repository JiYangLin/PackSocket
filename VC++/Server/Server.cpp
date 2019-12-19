// Server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include  "../TcpPack.hpp"


class Ser:IServerRecver
{
public:
	virtual void OnRec(ConnMark mark,char *recvBuf, int BufLen)
	{
		string str(recvBuf,recvBuf + BufLen);
		cout<<str<<endl;
	}
	void Run()
	{
		InitWinSocket();

		if(!m_ServerSocket.Start(1234,this))
		{
			cout<<"start Erro"<<endl;
			return;
		}

		string str;
		int mark;
		while(1)
		{
			cout<<"mark"<<endl;
			cin>>mark;
			cin.clear();

			cout<<"str"<<endl;
			cin>>str;

			m_ServerSocket.SendToMarkConn(mark,&str[0],str.length());//向指定链接发送

			//		m_ServerSocket.SendToAllConn(&str[0],str.length());      
		}
	}

	ServerSocket m_ServerSocket;
};


int main()
{
	Ser ser;
	ser.Run();
}
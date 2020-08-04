#include <iostream>
#include "TcpPack.h"

class Ser :IServerRecver
{
	string revStr;
public:
	virtual void OnRec(ConnMark mark, u_int32_t cmd, char* recvBuf, int BufLen)
	{
		revStr.resize(BufLen + 1);
		char* str = &revStr[0];
		memset(str, 0, BufLen + 1);
		memcpy(str, recvBuf, BufLen);
		cout << str << endl;
	}
	void Run()
	{
		cout << "======Server=======" << endl;

		if (!m_ServerSocket.Start(1234, this))
		{
			cout << "start Erro" << endl;
			return;
		}

		string str;
		int mark;
		while (1)
		{
			cout << "input client mark:" << endl;
			cin >> mark;

			cout << "input send data:" << endl;
			cin >> str;

			m_ServerSocket.SendToMarkConn(mark, 0, &str[0], str.length());
		}
	}

	ServerSocket m_ServerSocket;
};
class Cl :IClientRecver
{
	string revStr;
public:
	virtual void OnRec(u_int32_t cmd, char* recvBuf, int BufLen)
	{
		revStr.resize(BufLen + 1);
		char* str = &revStr[0];
		memset(str, 0, BufLen + 1);
		memcpy(str, recvBuf, BufLen);
		cout << str << endl;
	}
	void Run()
	{
		cout << "======client=======" << endl;

		cout << "input connect mark:" << endl;
		int mark = 0;
		cin >> mark;

		if (!m_ClientSocket.Start(mark, "127.0.0.1", 1234, this))
		{
			cout << "Connect Erro" << endl;
			return;
		}
		else
		{
			cout << "Connect suc" << endl;
		}

		string str;
		while (1)
		{
			cin >> str;
			m_ClientSocket.Send(0, &str[0], str.length());
		}
	}

	ClientSocket m_ClientSocket;
};




int main(int argc,char **argv)
{
	cout << "Proc Server?(Y/N)" << endl;
	string str;
	cin >> str;
	if (str == "Y" || str == "y")
	{
		Ser ser;
		ser.Run();
	}
	else
	{
		Cl cl;
		cl.Run();
	}
}
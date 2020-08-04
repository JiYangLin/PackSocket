#include "PackSocket2.h"
#include <iostream>
#include <string>
using namespace std;


void ShowRevMsgStr(PackSocketMSG *msg)
{
	string revStr;
	int strLen = msg->head.BigMsgLen + 1;
	revStr.resize(strLen);
	char* str = &revStr[0];
	memset(str, 0, strLen);
	memcpy(str, msg->data, msg->head.BigMsgLen);
	cout << str << endl;
}

class Ser :IServerRecver
{
public:
	virtual void OnRec(User *user, PackSocketMSG *msg, ClientDataBuf &clbuf)
	{
		ShowRevMsgStr(msg);
	}
	virtual bool UserCheck(User *user)
	{
		printf("Connectd : %d - %s\n", user->id, user->name);
		return true;
	}
	virtual void OnDisConnected(User *user, uint8_t errCode)
	{
		cout << "OnDisConnected " << endl;
	}


	void Run()
	{
		cout << "======Server=======" << endl;


		if (!m_ServerSocket.Start(1234, this,20))
		{
			cout << "start Erro" << endl;
			return;
		}

		cout << "start Succ" << endl;
		PackSocketMSG msg;
		string str;
		int id;
		while (1)
		{
			cout << "input client id:" << endl;
			cin >> id;

			cout << "input send id:" << endl;
			cin >> str;
			msg.head.cmd = 11;
			msg.head.BigMsgLen = str.length();
			msg.data = &str[0];
			m_ServerSocket.SendToMarkConn(id,&msg);
		}
	}

	ServerSocket m_ServerSocket;
};
class Cl :IClientRecver
{
public:
	virtual void OnDisConnected(uint8_t errCode){}
	virtual void OnRec(PackSocketMSG *msg)
	{
		ShowRevMsgStr(msg);
	}
	void Run()
	{
		cout << "======client=======" << endl;

		cout << "input connect id:" << endl;
		int id = 0;
		cin >> id;

		User user;
		user.answerMode = 0;
		user.id = id;
		strcpy(user.name,"client");


		if (!m_ClientSocket.Start(&user, "127.0.0.1", 1234, this))
		{
			cout << "Connect Erro" << endl;
			return;
		}
		else
		{
			cout << "Connect suc" << endl;
		}

		string str;
		PackSocketMSG msg;
		while (1)
		{
			cin >> str;

			msg.head.cmd = 22;
			msg.head.BigMsgLen = str.length();
			msg.data = &str[0];
			m_ClientSocket.Send(&msg);

		}
	}

	ClientSocket m_ClientSocket;
};

void main()
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
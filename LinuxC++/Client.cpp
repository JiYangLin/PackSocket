#include "TcpPack.h"

class Cl:IClientRecver
{
public:
	virtual void OnRec(char *recvBuf, int BufLen )
	{
		string str(recvBuf,recvBuf + BufLen);
		cout<<str<<endl;
	}
	void Run()
	{

		cout<<"client"<<endl;

		cout<<"mark:"<<endl;
		int mark = 0;
		cin>>mark;
		cin.clear();

		if(!m_ClientSocket.Start(mark,"127.0.0.1",1234,this))////////mark是连接标志
		{
			cout<<"Connect Erro"<<endl;
			return;
		}
		else
		{
			cout<<"Connect suc"<<endl;
		}

		string str;
		while(1)
		{
			cin>>str;
			m_ClientSocket.Send(&str[0],str.length());
		}
	}

	ClientSocket m_ClientSocket;
};

int main(int argc,char **argv)
{
	Cl cl;
	cl.Run();
}
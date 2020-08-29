#include "..\..\PackSocket\VC++\PkDemo\TcpPack.hpp"

class Client
{
	ClientSocket cl;
	char *m_ip;
	int m_port;
	int m_cmd;//0 file  1:msg
	int m_mark;
	list<string> m_data;


	bool Connect()
	{
		if (!cl.Start(m_mark, m_ip, m_port, 0))
		{
			cout << "connected err:" << m_ip << ":" << m_port << endl;
			return false;
		}
		return true;
	}
	void Err()
	{
		cout << endl;
		cout << "=====================" << endl;
		cout << "note: " << endl;
		cout << "ip:port [-mark 0] [-cmd 0]   dat..."<< endl;
		cout << "127.0.0.1:1234 file1.txt file2.txt file3.txt" << endl;
		cout << "127.0.0.1:1234 -cmd 1  msg...." << endl;
		cout << "=====================" << endl << endl;
	}


	void SendMsg()
	{
		if (!Connect()) return;
		string msg = *m_data.begin();
		if (!cl.Send(m_cmd, msg.c_str(), msg.length()))
		{
			cout << "send err" << endl;
		}
		RevBack(msg.length());
	}
	void SendFile()
	{
		if (!Connect()) return;
		for (auto iter = m_data.begin(); iter != m_data.end(); ++iter)
		{
			FILE* pf = NULL;
			fopen_s(&pf, (*iter).c_str(), "rb");
			if (NULL == pf)
			{
				cout << "open err :" << *iter << endl;
				continue;
			}
			fseek(pf, 0, SEEK_END);
			int len = ftell(pf);
			fseek(pf, 0, SEEK_SET);
			char* buf = new char[len];
			fread_s(buf, len, len, 1, pf);
			fclose(pf);
			cout << "start send..." << endl;
			if (!cl.Send(m_cmd, buf, len))
			{
				cout << "send err" << endl;
			}
			RevBack(len);
		}
	}
	void RevBack(int sendlen)
	{
		UINT32 cmdRev;
		int lenRev;
		char* bufRev = cl.ProcRev(cmdRev, lenRev);
		if (NULL == bufRev)
		{
			cout << "rev err" << endl;
			return;
		}
		char inf[100] = { 0 };
		memcpy_s(inf, 100, bufRev, lenRev);
		cout << inf << endl;
		cout << "send [" << sendlen << "] success!" << endl;
	}




	bool ParseArg(int argc, char** argv)
	{
		if (argc < 3)
		{
			Err();
			return false;
		}
		if (!ParseAddr(argv[1]))
		{
			cout << "addr err" << endl;
			Err();
			return false;
		}
		if(!ParseData(argc,argv))
		{
			cout << "no data" << endl;
			Err();
			return false;
		}
		return true;
	}
	bool ParseAddr(char *addr)
	{
		char* pos = strchr(addr,':');
		if (0 == pos) return false;
		*pos = 0;
		++pos;
		m_ip = addr;
		m_port = atoi(pos);
		return true;
	}
	int ParseDataParam(string param,int &i, int argc, char** argv)
	{
		int pos = i+1;
		if (param == "mark")
		{
			if (pos >= argc) return -1;
			m_mark = atoi(argv[pos]);
		}
		else if (param == "cmd")
		{
			if (pos >= argc) return -1;
			m_cmd = atoi(argv[pos]);
		}
		else
		{
			return 0;
		}
		return ++i;
	}
	bool ParseData(int argc, char** argv)
	{
		for (int i = 2; i < argc; ++i)
		{
			if (strlen(argv[i]) > 1)
			{
				if (argv[i][0] == '-')
				{
					string param = &argv[i][1];
					int code = ParseDataParam(param, i, argc, argv);
					if(-1 == code)
					{
						Err();
						return false;
					}
					else if(code>0)
					{
						continue;
					}
				}
			}		
			m_data.push_back(argv[i]);
		}
		return true;
	}
public:
	void Run(int argc, char** argv)
	{
		cout << "***** pack by jiyanglin *****" << endl << endl;
		if (!ParseArg(argc, argv)) return;

		if (m_cmd == 0) SendFile();
		else if (m_cmd == 1) SendMsg();
		else Err();
	}
};





int main(int argc, char** argv)
{
	Client().Run(argc, argv);
	return 0;
}

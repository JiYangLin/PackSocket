using PackSocket;
using System;
using System.Text;

namespace CSharp
{
    class Ser : IServerRev
    {
        void IServerRev.Rev(byte mark, uint cmd, byte[] recvBytes, int revlen)
        {
            if (null == recvBytes) return;

            string str = Encoding.Default.GetString(recvBytes, 0, revlen);

            Console.WriteLine(str);
        }
        public static int GetMark()
        {
            while(true)
            {
                try
                {
                   String str = Console.ReadLine();
                   int mark = Convert.ToInt32(str);
                    return mark;
                }catch(Exception e)
                {
                    Console.WriteLine("请输入正确数字");
                    continue;
                }
            }
              
        }
        public void Run()
        {

            m_SocketServer.Start(1234, this);

            Console.WriteLine("=====服务器====");

            string str;
            while (true)
            {
                Console.WriteLine("输入客户端 mark:");
                int mark = Ser.GetMark();

                Console.WriteLine("输入向客户端发送内容:");
                str = Console.ReadLine();
                byte[] bytes = Encoding.Default.GetBytes(str);
                m_SocketServer.SendToMarkConn((byte)mark, 0,bytes, (UInt32)bytes.Length);

            }
        }

        SocketServer m_SocketServer = new SocketServer();
    }

    class Client : IClientRev
    {
        void IClientRev.Rev(uint cmd, byte[] recvBytes, int revlen )
        {
            if (null == recvBytes) return;
            string str = Encoding.Default.GetString(recvBytes, 0, revlen);

            Console.WriteLine(str);
        }
        public void Run()
        {
            Console.WriteLine("=====客户端====");

         
            Console.WriteLine("输入连接 mark:");
            int mark = Ser.GetMark();
            m_SocketClient.Start((byte)mark, "127.0.0.1", 1234, this);


            while (true)
            {
                Console.WriteLine("输入向服务器发送内容:");
                String str = Console.ReadLine();
                byte[] bytes = Encoding.Default.GetBytes(str);

                m_SocketClient.Send(0,bytes, (UInt32)bytes.Length);
            }
        }

        SocketClient m_SocketClient = new SocketClient();
    }



    public class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Proc Server?(Y/N)");
            string str = Console.ReadLine();
	        if (str == "Y" || str == "y")
	        {
	        	Ser ser = new Ser();
	        	ser.Run();
	        }
	        else
	        {
	        	Client cl = new Client();
	        	cl.Run();
	        }
        }
    }
}

using PackSocket;
using System;
using System.Text;

namespace CSharp
{
    class Ser : IServerRev
    {
        void IServerRev.Rev(Byte mark, byte[] recvBytes, UInt32 revSize, string erroMsg)
        {
            if (null == recvBytes) return;

            string str = Encoding.Default.GetString(recvBytes, 0, (int)revSize + 1);

            Console.WriteLine(str);
        }
        public void Run()
        {

            m_SocketServer.Start(1234, this);

            Console.WriteLine("=====服务器====");

            string str;
            while (true)
            {
                Console.WriteLine("输入客户端 mark");

                str = Console.ReadLine();
                int mark = Convert.ToInt32(str);

                Console.WriteLine("输入向客户端发送内容 ");
                str = Console.ReadLine();
                byte[] bytes = Encoding.Default.GetBytes(str);
                m_SocketServer.SendToMarkConn((byte)mark, bytes, (UInt32)bytes.Length);

            }
        }

        SocketServer m_SocketServer = new SocketServer();
    }

    class Client : IClientRev
    {
        void IClientRev.Rev(byte[] recvBytes, UInt32 revSize, string erroMsg)
        {
            if (null == recvBytes) return;
            string str = Encoding.Default.GetString(recvBytes, 0, (int)revSize + 1);

            Console.WriteLine(str);
        }
        public void Run()
        {
            Console.WriteLine("=====客户端====");

            string str;
            Console.WriteLine("输入连接 mark");
            str = Console.ReadLine();
            int mark = Convert.ToInt32(str);

            m_SocketClient.Start((byte)mark, "127.0.0.1", 1234, this);


            while (true)
            {
                Console.WriteLine("str:");
                str = Console.ReadLine();
                byte[] bytes = Encoding.Default.GetBytes(str);

                m_SocketClient.Send(bytes, (UInt32)bytes.Length);
            }
        }

        SocketClient m_SocketClient = new SocketClient();
    }



    class Program
    {
        static void Main(string[] args)
        {
            //Ser ser = new Ser();
            //ser.Run();

            Client cl = new Client();
            cl.Run();
        }
    }
}

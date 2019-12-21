using MySocket;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


namespace Client
{
    class Client : IClientRev
    {
        void IClientRev.Rev(byte[] recvBytes, UInt32 revSize, string erroMsg)
        {
            if (null == recvBytes) return;
            string str = Encoding.Default.GetString(recvBytes,0, (int)revSize + 1);

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
            Client cl = new Client();
            cl.Run();
        }
    }
}

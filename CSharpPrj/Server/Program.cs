using MySocket;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    class Ser : IServerRev
    {
        void IServerRev.Rev(Byte mark, byte[] recvBytes, UInt32 revSize, string erroMsg)
        {
            if (null == recvBytes) return;

            string str = Encoding.Default.GetString(recvBytes);

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


    class Program
    {
        static void Main(string[] args)
        {
            Ser ss = new Ser();
            ss.Run();
        }
    }
}

using PackSocket;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


namespace CSharp
{
    class Ser : IRev
    {
        void IRev.onRevData(RevData data)
        {
            if (null == data) return;
            if (0 == data.msglen) return;

            string str = Encoding.Default.GetString(data.recvBytes, 0, data.msglen);

            Console.WriteLine(str);
        }

        public static int GetMark()
        {
            while (true)
            {
                try
                {
                    String str = Console.ReadLine();
                    int mark = Convert.ToInt32(str);
                    return mark;
                }
                catch (Exception e)
                {
                    Console.WriteLine("请输入正确数字");
                    continue;
                }
            }

        }
        public void Run()
        {
            Console.WriteLine("=====服务器====");


            bool localAddr = false;
            bool singleModel = false;

            Console.WriteLine("localAddr Server?(Y/N)");
            string str = Console.ReadLine();
            if (str == "Y" || str == "y") localAddr = true;

            Console.WriteLine("singleModel Server?(Y/N)");
            str = Console.ReadLine();
            if (str == "Y" || str == "y") singleModel = true;

            m_SocketServer.Start(1234, this, localAddr, singleModel);

    

            while (true)
            {
                Console.WriteLine("输入客户端 mark:");
                int mark = Ser.GetMark();

                Console.WriteLine("输入向客户端发送内容:");
                str = Console.ReadLine();
                byte[] bytes = Encoding.Default.GetBytes(str);
                m_SocketServer.SendToMarkConn((byte)mark, 0, bytes, (UInt32)bytes.Length);

            }
        }



        SocketServer m_SocketServer = new SocketServer();
    }

    class Client : IRev
    {
        void IRev.onRevData(RevData data)
        {
            if (null == data) return;
            if (0 == data.msglen) return;

            string str = Encoding.Default.GetString(data.recvBytes, 0, data.msglen);

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

                m_SocketClient.Send(0, bytes, (UInt32)bytes.Length);
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


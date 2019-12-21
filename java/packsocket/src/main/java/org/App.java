package org;

import java.util.Scanner;

class Ser implements IServerRev
{
    public void Rev(byte mark, byte[] recvBytes, int revSize, String erroMsg) {
        if (null == recvBytes) return;

        String str = new String(recvBytes,0,revSize);  
        System.out.println(str);
    }
    public void Run()
    {

        m_SocketServer.Start(1234, this);

        System.out.println("=====服务器====");

        String str;
        Scanner input = new Scanner(System.in);
        while (true)
        {
            System.out.println("输入客户端 mark");

            int mark = input.nextInt();

            System.out.println("输入向客户端发送内容 ");
            str = input.next();
            byte[] bytes = str.getBytes();

            m_SocketServer.SendToMarkConn((byte)mark, bytes, bytes.length);
        }
    }

    SocketServer m_SocketServer = new SocketServer();


}

class Client implements IClientRev
{
    public void Rev(byte[] recvBytes, int revSize, String erroMsg) {
        if (null == recvBytes)
            return;

        String str = new String(recvBytes, 0, revSize);
        System.out.println(str);
    }
    public void Run()
    {
        System.out.println("=====客户端====");

        Scanner input = new Scanner(System.in);
        String str;
        System.out.println("输入连接 mark");
    
        int mark = input.nextInt();

        m_SocketClient.Start((byte)mark, "localhost", 1234, this);


        while (true)
        {
            System.out.println("str:");
            str = input.next();
            byte[] bytes = str.getBytes();

            m_SocketClient.Send(bytes,bytes.length);
        }
    }

    SocketClient m_SocketClient = new SocketClient();
}

public class App 
{
    public static void main( String[] args )
    {
        //Ser ser = new Ser();
        //ser.Run();

        Client cl = new Client();
        cl.Run();
    }
}

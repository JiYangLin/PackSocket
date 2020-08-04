package org;

import java.util.Scanner;

class Ser implements IServerRev
{
    public void Rev(byte mark,int cmd, byte[] recvBytes, int revSize) {
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

            int mark = Ser.GetMark();

            System.out.println("输入向客户端发送内容 ");
            str = input.next();
            byte[] bytes = str.getBytes();

            m_SocketServer.SendToMarkConn((byte)mark,10, bytes, bytes.length);
        }
    }
    public static int GetMark()
    {
        Scanner input = new Scanner(System.in);
        while(true)
        {
            try
            {
               String str = input.next();
               int mark = Integer.parseInt(str);
                return mark;
            }catch(Exception e)
            {
                System.out.println("请输入正确数字");
                continue;
            }
        }
          
    }
    SocketServer m_SocketServer = new SocketServer();


}

class Client implements IClientRev
{
    public void Rev(int cmd,byte[] recvBytes, int revSize) {
        if (null == recvBytes)
            return;

        String str = new String(recvBytes, 0, revSize);
        System.out.println(str);
    }
    public void Run()
    {
        System.out.println("=====客户端====");


        String str;
        System.out.println("输入连接 mark");
    
        int mark = Ser.GetMark();
        m_SocketClient.Start((byte)mark, "localhost", 1234, this);

        Scanner input = new Scanner(System.in);
        while (true)
        {
            System.out.println("str:");
            str = input.next();
            byte[] bytes = str.getBytes();

            m_SocketClient.Send(10,bytes,bytes.length);
        }
    }

    SocketClient m_SocketClient = new SocketClient();
}

public class App 
{
    public static void main( String[] args )
    {

        System.out.println("Proc Server?(Y/N)");

        Scanner input = new Scanner(System.in);
        String str = input.next();
        if (str.equals("Y") || str.equals("y"))
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

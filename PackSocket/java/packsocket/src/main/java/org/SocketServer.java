package org;

import java.net.*;
import java.util.ArrayList;

public class SocketServer extends Thread
{
    ServerSocket socketServer = null;
    Thread threadSocket = null;
    IRev revProc = null;
    ArrayList<Connect> SocketConnect = new ArrayList<Connect>();
    Object lock = new Object();
    public void StartServer(int port, IRev _revProc)
    {
        try
        {
            revProc = _revProc;
            socketServer = new ServerSocket(port);
    
            threadSocket = new Thread(this);
            threadSocket.setDaemon(true);
            threadSocket.start();
        }catch(Exception e)
        {
            System.out.println(e);
        }

    }
    public void run(){
        while (true)
        {
            try
            {
                Socket connSocket = socketServer.accept();

                synchronized(lock){      
                    RemoveErroConnect();
                    Connect conn = new Connect(connSocket, revProc, null);
                    SocketConnect.add(conn);   
                }
            }catch(Exception e)
            {
                System.out.println(e);
            }     
        }
    }

    
    public void SendToAllConn(int cmd,byte[] msg, int msgSize)
    {
        SendToConn(cmd,msg, msgSize,Param.ConnMark_Def);
    }
    public void SendToMarkConn(byte mark,int cmd, byte[] msg, int msgSize)
    {
        SendToConn(cmd,msg, msgSize, mark);
    }
    void SendToConn(int cmd,byte[] msg, int msgSize, byte mark)
    {
        synchronized(lock){      
        for (Connect conn : SocketConnect)
        {
            if (!conn.connectNormal) continue;
            if (mark != (Byte)Param.ConnMark_Def)
                if (mark != conn.GetConnMark()) continue;
            conn.Send(cmd,msg, msgSize);
        }
        }
    }
    void RemoveErroConnect()
    {
        SocketConnect.removeIf(conn->conn.connectNormal == false);
    }
}
package org;

import java.io.DataOutputStream;
import java.net.*;

public class SocketClient
{
    Connect m_Connect = null;
    public void Start(byte mark, String IP, int prort, IClientRev revProc)
    {
        try
        {
            Socket Client = new Socket(IP, prort);

            byte[] by = new byte[1];
            by[0] = mark;
            DataOutputStream data = new DataOutputStream(Client.getOutputStream());
            data.write(by);
            m_Connect = new Connect(Client, null, revProc);
        }catch(Exception e)
        {
            System.out.println(e);
        }
    }
    public boolean Send(int cmd,byte[] msg, int msgSize)
    {
        if (null == m_Connect) return false;
        return m_Connect.Send(cmd,msg, msgSize);
    }
}
package org;

import java.io.*;
import java.net.*;

import org.MsgCtl.IRevFinish;

public class Connect extends Thread implements IRevFinish
{
    public void RevFinish(byte[] revByte, int revSize) {
        Rev(revByte, revSize, "");
    }


    Socket conn = null;
    Thread revThread = null;
    IServerRev m_IServerRev = null;
    IClientRev m_IClientRev = null;
    MsgCtl m_MsgCtl = new MsgCtl();
    byte m_ConnMark = (byte)Param.ConnMark_Def;
    newMsgMark m_newMsgMark = new newMsgMark();
    public boolean connectNormal = false;
    public byte GetConnMark() { return m_ConnMark; }
    public void Close()
    {
        try
        {
            conn.close();
        }catch(Exception e)
        {
            System.out.println(e);
        }
    }
    public Connect(Socket _conn, IServerRev _IServerRev, IClientRev _IClientRev)
    {
        m_IServerRev = _IServerRev;
        m_IClientRev = _IClientRev;
        connectNormal = true;
        conn = _conn;
        revThread = new Thread(this);
        revThread.setDaemon(true);
        revThread.start();
    }
    public void run(){
        try
        {
            OnRev();
        }
        catch (Exception e)
        {
            System.out.println(e);
            //错误时接收数据为null作为通知
            Rev(null, 0, e.toString());
            connectNormal = false;
        }
    }

    void Rev(byte[] msg, int msgSize, String erro)
    {
        if (null != m_IServerRev) m_IServerRev.Rev(m_ConnMark, msg, msgSize, erro);
        else m_IClientRev.Rev(msg, msgSize, erro);
    }
    void OnRev() throws Exception
    {
        byte[] recvBytes = new byte[(int)Param.REV_LEN];
        int recBufPos = 0;
        int BufLen = 0;
        InputStream in = conn.getInputStream();
        if (m_IServerRev != null)
        {
            int len = in.read(recvBytes,0,Param.MarkLen);
            m_ConnMark = recvBytes[0];
           
        }
        while (true)
        {
            BufLen = in.read(recvBytes, recBufPos, Param.REV_LEN - recBufPos);
            recBufPos += BufLen;

            ////接收到足够一帧大小 Param.REV_LEN 后再执行
            if (recBufPos != (int)Param.REV_LEN) continue;
            recBufPos = 0;
            m_MsgCtl.Rev(recvBytes, this);
        }
    }
    public boolean Send(byte[] msg, int msgSize)
    {
        try
        {
            m_MsgCtl.Send(conn, msg, msgSize);
        }
        catch (Exception ex)
        {
            return false;
        }
        return true;
    }


}
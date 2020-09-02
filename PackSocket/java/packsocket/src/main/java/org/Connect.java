package org;

import java.io.*;
import java.net.*;

public class Connect extends Thread implements IRev
{    
    @Override
    public void onRevData(RevData revdata) {
        revdata.mark = m_ConnMark;
        if (null != m_IServerRev) m_IServerRev.onRevData(revdata);
        else m_IClientRev.onRevData( revdata);
    }


    Socket conn = null;
    Thread revThread = null;
    IRev m_IServerRev = null;
    IRev m_IClientRev = null;
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
    public Connect(Socket _conn, IRev iServerRev, IRev iClientRev)
    {
        m_IServerRev = iServerRev;
        m_IClientRev = iClientRev;
        connectNormal = true;
        conn = _conn;
        if(null == iServerRev && null == iClientRev) return;
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
            onRevData(null);
            connectNormal = false;
        }
    }
    void OnRev() throws Exception
    {
        byte[] recvBytes = new byte[(int)Param.REV_LEN];
        int recBufPos = 0;
        int BufLen = 0;
        InputStream in = conn.getInputStream();
        if (m_IServerRev != null)
        {
            in.read(recvBytes,0,Param.MarkLen);
            m_ConnMark = recvBytes[0];
           
        }
        while (true)
        {
            BufLen = in.read(recvBytes, recBufPos, Param.REV_LEN - recBufPos);
            recBufPos += BufLen;

            if (recBufPos != (int)Param.REV_LEN) continue;
            recBufPos = 0;
            m_MsgCtl.Rev(recvBytes, this);
        }
    }
    public boolean Send(int cmd,byte[] msg, int msgSize)
    {
        try
        {
            m_MsgCtl.Send(conn, cmd,msg, msgSize);
        }
        catch (Exception ex)
        {
            return false;
        }
        return true;
    }



}
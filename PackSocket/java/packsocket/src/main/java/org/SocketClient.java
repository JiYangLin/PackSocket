package org;

import java.io.DataOutputStream;
import java.io.InputStream;
import java.net.*;


public class SocketClient implements IRev
{
    Connect mConnect = null;
    MsgCtl mMsgCtl = new MsgCtl();
    Socket mClient  = new Socket();

    public RevData m_revdata = new RevData();

    public void Start(byte mark, String IP, int prort, IRev revProc)
    {
        try
        {
            mClient = new Socket(IP, prort);

            byte[] by = new byte[1];
            by[0] = mark;
            DataOutputStream data = new DataOutputStream(mClient.getOutputStream());
            data.write(by);
            mConnect = new Connect(mClient, null, revProc);
        }catch(Exception e)
        {
            System.out.println(e);
        }
    }
    public boolean Send(int cmd,byte[] msg, int msgSize)
    {
        if (null == mConnect) return false;
        return mConnect.Send(cmd,msg, msgSize);
    }
    public RevData ProcRev()
    {
        try
        {
            m_revdata.revByte = null;
            byte[] recvBytes = new byte[(int)Param.REV_LEN];
            int recBufPos = 0;
            int BufLen = 0;
            InputStream in = mClient.getInputStream();
            while (true)
            {
                BufLen = in.read(recvBytes, recBufPos, Param.REV_LEN - recBufPos);
                recBufPos += BufLen;
    
                if (recBufPos != (int)Param.REV_LEN) continue;
                recBufPos = 0;
                mMsgCtl.Rev(recvBytes,this);
                if( m_revdata.revByte != null) return m_revdata;
            }
        }catch(Exception e)
        {
            System.out.println(e.toString());
            return null;
        }
    }

    @Override
    public void onRevData(RevData revdata) {
        m_revdata.cmd = revdata.cmd;
        m_revdata.revByte = revdata.revByte;
        m_revdata.revlen = revdata.revlen;
    }
}
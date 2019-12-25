package org;

import java.io.*;
import java.net.*;

public class MsgCtl
{
    interface IRevFinish{

        public void RevFinish(int cmd,byte[] revByte,int revlen);
    }


    class Receiver
    {
        int m_revSize = 0;
        int m_msgSize = 0;
        int m_cmd = 0;
        byte[] m_buf = new byte[Param.REV_LEN];
        newMsgMark m_newMsgMark = new newMsgMark();

        public void rev(byte[] recvBuf, IRevFinish Fun)
        {
            if (m_newMsgMark.isNewMsgMark(recvBuf))
            {
                m_revSize = 0;
                m_msgSize = Param.byte4ToInt(recvBuf, Param.GUID_LEN);
                m_cmd = Param.byte4ToInt(recvBuf, Param.Pos_cmd);
                if(m_msgSize > m_buf.length) m_buf = new byte[m_msgSize];

                if (m_msgSize <= Param.newMsgMaxDataSpace)
                {
                    System.arraycopy(recvBuf, Param.Pos_msg, m_buf, 0,m_msgSize);
                    MsgFinish(Fun);
                }
                return;
            }


            int CpySize = Param.REV_LEN;
            if (m_revSize + Param.REV_LEN > m_msgSize) CpySize = (int)(m_msgSize - m_revSize);

            System.arraycopy(recvBuf,0,m_buf,m_revSize,CpySize);
            m_revSize = m_revSize + CpySize;

            if (m_revSize != m_msgSize) return;

            MsgFinish(Fun);
        }
        void MsgFinish(IRevFinish Fun)
        {
            Fun.RevFinish(m_cmd,m_buf,m_msgSize);
            m_msgSize = 0;
            m_revSize = 0;
        }
    }
    class Sender
    {
        byte[] SendBuf = new byte[(int)Param.REV_LEN];
        newMsgMark m_newMsgMark = new newMsgMark();

        public void SendMsg(Socket sock, int cmd,byte[] msg, int msgSize)
        {
            try
            {
                _SendMsg(sock,cmd,msg,msgSize);
            }catch(Exception e)
            {
                System.out.println(e);
            }
            
        }
        void _SendMsg(Socket sock,int cmd, byte[] msg, int msgSize) throws IOException
        {
            System.arraycopy(m_newMsgMark.NewMsgMarkStr,0,SendBuf,0,Param.GUID_LEN);
            byte[] sizeBytes = Param.intToByte4(msgSize); 
            System.arraycopy(sizeBytes,0,SendBuf,Param.GUID_LEN,sizeBytes.length);
            byte[] cmdBytes = Param.intToByte4(cmd);
            System.arraycopy(cmdBytes,0,SendBuf,Param.Pos_cmd,cmdBytes.length);
            System.arraycopy(m_newMsgMark.NewMsgMarkStrEnd,0,SendBuf,Param.Pos_NewMsgMarkStrEnd,Param.GUID_LEN);

            if (msgSize <= Param.newMsgMaxDataSpace)
            {
                System.arraycopy(msg,0,SendBuf,Param.Pos_msg,msgSize);
                DataOutputStream data = new DataOutputStream(sock.getOutputStream());
                data.write(SendBuf);
                return;
            }


            DataOutputStream data = new DataOutputStream(sock.getOutputStream());
            data.write(SendBuf);

            int hasSendSize = 0;
            while (true)
            {
                if (msgSize <= (int)Param.REV_LEN)
                {
                    System.arraycopy(msg,hasSendSize,SendBuf,0,msgSize);
                    data.write(SendBuf);
                    return;
                }

                System.arraycopy(msg,hasSendSize,SendBuf,0,Param.REV_LEN);
                data.write(SendBuf);

                hasSendSize += Param.REV_LEN;
                msgSize -= Param.REV_LEN;
            }
        }
    }



    public void Rev(byte[] recvBuf, IRevFinish Fun)
    {
        m_Receiver.rev(recvBuf, Fun);
    }
    public void Send(Socket sock, int cmd,byte[] msg, int msgSize)
    {
        m_Sender.SendMsg(sock, cmd,msg, msgSize);
    }
    Receiver m_Receiver = new Receiver();
    Sender m_Sender = new Sender();

}
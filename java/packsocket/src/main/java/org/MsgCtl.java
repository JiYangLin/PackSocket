package org;

import java.io.*;  
import java.net.*;  

public class MsgCtl
{
    interface IRevFinish{

        public void RevFinish(byte[] revByte, int revSize);
    }


    class Receiver
    {
        int m_revSize = 0;
        int m_msgSize = 0;
        byte[] m_buf = new byte[Param.REV_LEN];
        newMsgMark m_newMsgMark = new newMsgMark();

        public void rev(byte[] recvBuf, IRevFinish Fun)
        {
            if (m_newMsgMark.isNewMsgMark(recvBuf))
            {
                m_revSize = 0;
                m_msgSize = Param.byte4ToInt(recvBuf, Param.GUID_LEN);
                if (m_buf.length < m_msgSize) m_buf = new byte[m_msgSize];
                return;
            }


            int CpySize = (int)Param.REV_LEN;
            if (m_revSize + (int)Param.REV_LEN > m_msgSize) CpySize = (int)(m_msgSize - m_revSize);


            if (0 == m_msgSize) return;
            if (m_buf.length < m_revSize + CpySize) return;

            for(int i = 0 ; i< CpySize;++i) m_buf[m_revSize+i] = recvBuf[i];
            m_revSize = m_revSize + CpySize;


            if (m_revSize != m_msgSize) return;

            Fun.RevFinish(m_buf, m_revSize);
            m_msgSize = 0;
            m_revSize = 0;
        }
    }
    class Sender
    {
        byte[] SendBuf = new byte[(int)Param.REV_LEN];
        newMsgMark m_newMsgMark = new newMsgMark();

        public void SendMsg(Socket sock, byte[] msg, int msgSize)
        {
            try
            {
                _SendMsg(sock,msg,msgSize);
            }catch(Exception e)
            {
                System.out.println(e);
            }
            
        }
        void _SendMsg(Socket sock, byte[] msg, int msgSize) throws IOException
        {
            for(int i = 0 ; i < Param.REV_LEN;++i) SendBuf[i] = 0;

            for(int i = 0 ; i < Param.GUID_LEN;++i) SendBuf[i] = m_newMsgMark.NewMsgMarkStr[i];
            byte[] sizeBytes = Param.intToByte4(msgSize);
            
            for(int i = 0 ; i < sizeBytes.length;++i) SendBuf[i+Param.GUID_LEN] = sizeBytes[i];
            int pos = Param.REV_LEN - Param.GUID_LEN;
            for(int i = 0 ; i < Param.GUID_LEN;++i) SendBuf[i+pos] = m_newMsgMark.NewMsgMarkStrEnd[i];

            DataOutputStream data = new DataOutputStream(sock.getOutputStream());
            data.write(SendBuf);

            int hasSendSize = 0;
            while (true)
            {
                for(int i = 0 ; i < Param.REV_LEN;++i) SendBuf[i] = 0;

                if (msgSize <= (int)Param.REV_LEN)
                {
                    for(int i = 0 ; i < msgSize;++i) SendBuf[i] = msg[i+hasSendSize];
                    data.write(SendBuf);
                    return;
                }

                for(int i = 0 ; i < Param.REV_LEN;++i) SendBuf[i] = msg[i+hasSendSize];
                data.write(SendBuf);


                hasSendSize += (int)Param.REV_LEN;
                msgSize -= (int)Param.REV_LEN;
            }
        }
    }



    public void Rev(byte[] recvBuf, IRevFinish Fun)
    {
        m_Receiver.rev(recvBuf, Fun);
    }
    public void Send(Socket sock, byte[] msg, int msgSize)
    {
        m_Sender.SendMsg(sock, msg, msgSize);
    }
    Receiver m_Receiver = new Receiver();
    Sender m_Sender = new Sender();

}
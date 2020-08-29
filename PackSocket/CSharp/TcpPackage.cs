using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;



namespace PackSocket
{
    public interface IServerRev
    {
        void Rev(byte mark, uint cmd, byte[] recvBytes,int revlen);
    }
    public interface IClientRev
    {
        void Rev(uint cmd, byte[] recvBytes,int revlen);
    }

    class Param
    {
        public static int REV_LEN = 1024;
        public static int MAX_LISTEN_NUM = 20;

        public static int GUID_LEN = 16;

        public static int NewMsg_Size_LEN = 4;
        public static int CMD_LEN = 4;

        public static int Pos_cmd = 20;
        public static int Pos_msg = 24;
        public static int Pos_NewMsgMarkStrEnd = 1008;
        public static int newMsgMaxDataSpace = 984;

        public static int MarkLen = 1;
    }
    class newMsgMark
    {
        public newMsgMark()
        {
            string _NewMsgMarkStr = "4A6959616E674C696E6DB62435625348";
            NewMsgMarkStr = Encoding.Default.GetBytes(_NewMsgMarkStr);

            string _NewMsgMarkStrEnd = "414B6D9D2AA1C14A6959616E674C696E";
            NewMsgMarkStrEnd = Encoding.Default.GetBytes(_NewMsgMarkStrEnd);
        }
        public byte[] NewMsgMarkStr { get; set; }
        public byte[] NewMsgMarkStrEnd { get; set; }
        public bool isNewMsgMark(byte[] recvBuf)
        {
            for (int i = 0; i < Param.GUID_LEN; ++i)
                if (NewMsgMarkStr[i] != recvBuf[i]) return false;

            for (int i = 0; i < Param.GUID_LEN; ++i)
                if (NewMsgMarkStrEnd[i] != recvBuf[i + Param.Pos_NewMsgMarkStrEnd]) return false;

            return true;
        }
    }

    delegate void RevFinish(uint cmd, byte[] revByte,int revlen);
    class MsgCtl
    {
        class Receiver
        {
            uint m_revSize = 0;
            uint m_msgSize = 0;
            uint m_cmd = 0;
            byte[] m_buf = new byte[Param.REV_LEN];
            newMsgMark m_newMsgMark = new newMsgMark();

            public void rev(byte[] recvBuf, RevFinish Fun)
            {
                if (m_newMsgMark.isNewMsgMark(recvBuf))
                {
                    m_revSize = 0;
                    m_msgSize = BitConverter.ToUInt32(recvBuf, Param.GUID_LEN);
                    m_cmd = BitConverter.ToUInt32(recvBuf, Param.Pos_cmd );
                    if(m_msgSize > m_buf.Length) m_buf = new byte[m_msgSize];

                    if (m_msgSize <= Param.newMsgMaxDataSpace)
                    {
                        Array.Copy(recvBuf, Param.Pos_msg, m_buf, 0, m_msgSize);
                        MsgFinish(Fun);
                    }
                    return;
                }


                int CpySize = Param.REV_LEN;
                if (m_revSize + Param.REV_LEN > m_msgSize) CpySize = (int)(m_msgSize - m_revSize);

                Array.Copy(recvBuf, 0, m_buf, m_revSize, CpySize);
                m_revSize = m_revSize + (uint)CpySize;

                if (m_revSize != m_msgSize) return;

                MsgFinish(Fun);
            }
            void MsgFinish(RevFinish Fun)
            {
                Fun(m_cmd, m_buf,(int)m_msgSize);
                m_msgSize = 0;
                m_revSize = 0;
            }
        }
        class Sender
        {
            byte[] SendBuf = new byte[Param.REV_LEN];
            newMsgMark m_newMsgMark = new newMsgMark();

            public void SendMsg(Socket sock, uint cmd, byte[] msg, uint msgSize)
            {
                Array.Copy(m_newMsgMark.NewMsgMarkStr, 0, SendBuf, 0, Param.GUID_LEN);
                byte[] sizeBytes = BitConverter.GetBytes(msgSize);
                Array.Copy(sizeBytes, 0, SendBuf, Param.GUID_LEN, sizeBytes.Length);
                byte[] cmdBytes = BitConverter.GetBytes(cmd);
                Array.Copy(cmdBytes, 0, SendBuf, Param.Pos_cmd, cmdBytes.Length);
                Array.Copy(m_newMsgMark.NewMsgMarkStrEnd, 0, SendBuf, Param.Pos_NewMsgMarkStrEnd, Param.GUID_LEN);

                if (msgSize <= Param.newMsgMaxDataSpace)
                {
                    Array.Copy(msg, 0, SendBuf, Param.Pos_msg, msgSize);
                    sock.Send(SendBuf, Param.REV_LEN, 0);
                    return;
                }
                sock.Send(SendBuf, Param.REV_LEN, 0);

                uint hasSendSize = 0;
                while (true)
                {
                    if (msgSize <= Param.REV_LEN)
                    {
                        Array.Copy(msg, hasSendSize, SendBuf, 0, msgSize);
                        sock.Send(SendBuf, Param.REV_LEN, 0);
                        return;
                    }


                    Array.Copy(msg, hasSendSize, SendBuf, 0, Param.REV_LEN);
                    sock.Send(SendBuf, Param.REV_LEN, 0);

                    hasSendSize += (uint)Param.REV_LEN;
                    msgSize -= (uint)Param.REV_LEN;
                }
            }
        }



        public void Rev(byte[] recvBuf, RevFinish Fun)
        {
            m_Receiver.rev(recvBuf, Fun);
        }
        public void Send(Socket sock, uint cmd, byte[] msg, uint msgSize)
        {
            m_Sender.SendMsg(sock, cmd, msg, msgSize);
        }
        Receiver m_Receiver = new Receiver();
        Sender m_Sender = new Sender();
    }


    class Connect
    {
        Socket conn = null;
        Thread revThread = null;
        IServerRev m_IServerRev = null;
        IClientRev m_IClientRev = null;
        MsgCtl m_MsgCtl = new MsgCtl();
        byte m_ConnMark = 0xFF;
        newMsgMark m_newMsgMark = new newMsgMark();
        public bool connectNormal { get; set; }
        public byte GetConnMark() { return m_ConnMark; }

        public void Close()
        {
            conn.Close();
        }
        public Connect(Socket _conn, IServerRev _IServerRev, IClientRev _IClientRev)
        {
            m_IServerRev = _IServerRev;
            m_IClientRev = _IClientRev;

            connectNormal = true;
            conn = _conn;

            revThread = new Thread(_OnRev);
            revThread.IsBackground = true;
            revThread.Start();
        }


        void Rev(uint cmd, byte[] msg,int msglen)
        {
            if (null != m_IServerRev) m_IServerRev.Rev(m_ConnMark, cmd, msg, msglen);
            else m_IClientRev.Rev(cmd, msg, msglen);
        }
        void _OnRev()
        {
            try
            {
                OnRev();
            }
            catch (Exception e)
            {
                Rev(0xFFFFFFFF, null, 0);
                connectNormal = false;
            }
        }

        void OnRev()
        {
            byte[] recvBytes = new byte[Param.REV_LEN];
            int recBufPos = 0;
            int BufLen = 0;

            if (m_IServerRev != null)
            {
                conn.Receive(recvBytes, 0, Param.MarkLen, 0);
                m_ConnMark = recvBytes[0];
            }

            while (true)
            {

                BufLen = conn.Receive(recvBytes, recBufPos, Param.REV_LEN - recBufPos, 0);
                if(BufLen <=0 ) throw new Exception("dis connect");
				recBufPos += BufLen;

                if (recBufPos != Param.REV_LEN) continue;
                recBufPos = 0;

                m_MsgCtl.Rev(recvBytes, (uint cmd, byte[] revByte,int revLen) =>
                {
                    Rev(cmd, revByte, revLen);
                });
            }
        }
        public bool Send(uint cmd, byte[] msg, uint msgSize)
        {
            try
            {
                m_MsgCtl.Send(conn, cmd, msg, msgSize);
            }
            catch (Exception ex)
            {
                return false;
            }
            return true;

        }
    }
}



//服务端
namespace PackSocket
{
    public class SocketServer
    {
        Thread threadSocket = null;
        Socket socketServer = null;
        IServerRev revProc = null;
        List<Connect> SocketConnect = new List<Connect>();
        Mutex mutex = new Mutex();

        public void Start(int port, IServerRev _revProc)
        {
            revProc = _revProc;

            socketServer = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            IPEndPoint endpoint = new IPEndPoint(IPAddress.Any, port);
            socketServer.Bind(endpoint);

            threadSocket = new Thread(Accept);
            threadSocket.IsBackground = true;
            threadSocket.Start();
        }
        public void SendToAllConn(uint cmd, byte[] msg, uint msgSize)
        {
            SendToConn(cmd, msg, msgSize);
        }
        public void SendToMarkConn(byte mark, uint cmd, byte[] msg, uint msgSize)
        {
            SendToConn(cmd, msg, msgSize, mark);
        }
        void SendToConn(uint cmd, byte[] msg, uint msgSize, byte mark = 0xFF)
        {
            mutex.WaitOne();
            foreach (Connect conn in SocketConnect)
            {
                if (!conn.connectNormal) continue;

                if (mark != 0xFF)
                    if (mark != conn.GetConnMark()) continue;
                conn.Send(cmd, msg, msgSize);
            }
            mutex.ReleaseMutex();
        }

        void Accept()
        {
            socketServer.Listen(Param.MAX_LISTEN_NUM);
            while (true)
            {
                Socket connSocket = socketServer.Accept();

                mutex.WaitOne();
                RemoveErroConnect();
                Connect conn = new Connect(connSocket, revProc, null);
                SocketConnect.Add(conn);
                mutex.ReleaseMutex();
            }
        }

        void RemoveErroConnect()
        {
            SocketConnect.RemoveAll((Connect conn) =>
            {
                if (!conn.connectNormal)
                {
                    conn.Close();
                }
                return !conn.connectNormal;
            });

        }
    }
}


//客户端
namespace PackSocket
{
    public class SocketClient
    {
        Connect m_Connect = null;
        public void Start(byte mark, string IP, int prort, IClientRev revProc)
        {
            IPAddress address = IPAddress.Parse(IP);
            IPEndPoint endpoint = new IPEndPoint(address, prort);
            Socket Client = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);


            Client.Connect(endpoint);
            byte[] by = new byte[1];
            by[0] = mark;
            Client.Send(by, 1, 0);
            m_Connect = new Connect(Client, null, revProc);


        }
        public bool Send(uint cmd, byte[] msg, uint msgSize)
        {
            if (null == m_Connect) return false;
            return m_Connect.Send(cmd, msg, msgSize);
        }
    }
}
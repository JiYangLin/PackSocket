using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;



namespace PackSocket
{
    class param
    {
        public static int socketRevBufSize = 1024;
        public static int MAX_LISTEN_NUM = 20;

        public static int guidLEN = 16;

        public static int newMsgSizeLen = 4;
        public static int cmdLen = 4;

        public static int posCmd = 20;
        public static int posMsg = 24;
        public static int posNewMsgMarkStrEnd = 1008;
        public static int newMsgMaxDataSpace = 984;
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
            for (int i = 0; i < param.guidLEN; ++i)
                if (NewMsgMarkStr[i] != recvBuf[i]) return false;

            for (int i = 0; i < param.guidLEN; ++i)
                if (NewMsgMarkStrEnd[i] != recvBuf[i + param.posNewMsgMarkStrEnd]) return false;

            return true;
        }
    }



    public class RevData
    {
        public byte mark = 0;
        public uint cmd = 0;
        public byte[] recvBytes = new byte[param.socketRevBufSize];
        public int msglen = 0;
    }
    public interface IRev
    {
        void onRevData(RevData data);
    }




    class receiver
    {
        uint mRevSize = 0;
        RevData mRevData = new RevData();
        newMsgMark mNewMsgMark = new newMsgMark();

        public void rev(byte[] recvBuf, IRev Fun)
        {
            if (mNewMsgMark.isNewMsgMark(recvBuf))
            {
                mRevSize = 0;
                mRevData.msglen = (int)BitConverter.ToUInt32(recvBuf, param.guidLEN);
                mRevData.cmd = BitConverter.ToUInt32(recvBuf, param.posCmd);
                if (mRevData.msglen > mRevData.recvBytes.Length) mRevData.recvBytes = new byte[mRevData.msglen];

                if (mRevData.msglen <= param.newMsgMaxDataSpace)
                {
                    Array.Copy(recvBuf, param.posMsg, mRevData.recvBytes, 0, mRevData.msglen);
                    MsgFinish(Fun);
                }
                return;
            }


            int CpySize = param.socketRevBufSize;
            if (mRevSize + param.socketRevBufSize > mRevData.msglen) CpySize = (int)(mRevData.msglen - mRevSize);

            Array.Copy(recvBuf, 0, mRevData.recvBytes, mRevSize, CpySize);
            mRevSize = mRevSize + (uint)CpySize;

            if (mRevSize != mRevData.msglen) return;

            MsgFinish(Fun);
        }
        void MsgFinish(IRev Fun)
        {
            Fun.onRevData(mRevData);
            mRevData.msglen = 0;
            mRevSize = 0;
        }
    }
    class sender
    {
        byte[] mSendBuf = new byte[param.socketRevBufSize];
        newMsgMark mNewMsgMark = new newMsgMark();

        public void SendMsg(Socket sock, uint cmd, byte[] msg, uint msgSize)
        {
            Array.Copy(mNewMsgMark.NewMsgMarkStr, 0, mSendBuf, 0, param.guidLEN);
            byte[] sizeBytes = BitConverter.GetBytes(msgSize);
            Array.Copy(sizeBytes, 0, mSendBuf, param.guidLEN, sizeBytes.Length);
            byte[] cmdBytes = BitConverter.GetBytes(cmd);
            Array.Copy(cmdBytes, 0, mSendBuf, param.posCmd, cmdBytes.Length);
            Array.Copy(mNewMsgMark.NewMsgMarkStrEnd, 0, mSendBuf, param.posNewMsgMarkStrEnd, param.guidLEN);

            if (msgSize <= param.newMsgMaxDataSpace)
            {
                Array.Copy(msg, 0, mSendBuf, param.posMsg, msgSize);
                sock.Send(mSendBuf, param.socketRevBufSize, 0);
                return;
            }
            sock.Send(mSendBuf, param.socketRevBufSize, 0);

            uint hasSendSize = 0;
            while (true)
            {
                if (msgSize <= param.socketRevBufSize)
                {
                    Array.Copy(msg, hasSendSize, mSendBuf, 0, msgSize);
                    sock.Send(mSendBuf, param.socketRevBufSize, 0);
                    return;
                }


                Array.Copy(msg, hasSendSize, mSendBuf, 0, param.socketRevBufSize);
                sock.Send(mSendBuf, param.socketRevBufSize, 0);

                hasSendSize += (uint)param.socketRevBufSize;
                msgSize -= (uint)param.socketRevBufSize;
            }
        }
    }


    class connect : IRev
    {
        public Socket mConn = null;
        IRev mIServerRev = null;
        IRev mIClientRev = null;
        receiver mReceiver = new receiver();
        sender mSender = new sender();
        public byte mConnMark = 0xFF;
        newMsgMark mNewMsgMark = new newMsgMark();
        public bool mConnectNormal = false;

        public void Close()
        {
            mConn.Close();
        }
        public connect(Socket _conn, IRev _IServerRev, IRev _IClientRev)
        {
            mIServerRev = _IServerRev;
            mIClientRev = _IClientRev;

            mConnectNormal = true;
            mConn = _conn;

            if (null == _IClientRev && null == mIServerRev) return;
            Thread revThread = new Thread(_OnRev);
            revThread.IsBackground = true;
            revThread.Start();
        }

        public void onRevData(RevData data)
        {
            data.mark = mConnMark;
            if (null != mIServerRev) mIServerRev.onRevData(data);
            else mIClientRev.onRevData(data);
        }
        void _OnRev()
        {
            try
            {
                OnRev();
            }
            catch (Exception)
            {
                onRevData(null);
                mConnectNormal = false;
            }
        }

        void OnRev()
        {
            byte[] recvBytes = new byte[param.socketRevBufSize];
            int recBufPos = 0;
            int BufLen = 0;

            if (mIServerRev != null)
            {
                mConn.Receive(recvBytes, 0, 1, 0);
                mConnMark = recvBytes[0];
            }

            while (true)
            {

                BufLen = mConn.Receive(recvBytes, recBufPos, param.socketRevBufSize - recBufPos, 0);
                if (BufLen <= 0) throw new Exception("dis connect");
                recBufPos += BufLen;

                if (recBufPos != param.socketRevBufSize) continue;
                recBufPos = 0;

                mReceiver.rev(recvBytes, this);
            }
        }
        public bool Send(uint cmd, byte[] msg, uint msgSize)
        {
            try
            {
                mSender.SendMsg(mConn, cmd, msg, msgSize);
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
        Socket mSocketServer = null;
        IRev mRevProc = null;
        List<connect> mSocketConnect = new List<connect>();
        Mutex mMutex = new Mutex();

        public void Start(int port, IRev _revProc)
        {
            mRevProc = _revProc;

            mSocketServer = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            IPEndPoint endpoint = new IPEndPoint(IPAddress.Any, port);
            mSocketServer.Bind(endpoint);

            Thread threadSocket = new Thread(Accept);
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
            mMutex.WaitOne();
            foreach (connect conn in mSocketConnect)
            {
                if (!conn.mConnectNormal) continue;

                if (mark != 0xFF)
                    if (mark != conn.mConnMark) continue;
                conn.Send(cmd, msg, msgSize);
            }
            mMutex.ReleaseMutex();
        }

        void Accept()
        {
            mSocketServer.Listen(param.MAX_LISTEN_NUM);
            while (true)
            {
                Socket connSocket = mSocketServer.Accept();

                mMutex.WaitOne();
                RemoveErroConnect();
                connect conn = new connect(connSocket, mRevProc, null);
                mSocketConnect.Add(conn);
                mMutex.ReleaseMutex();
            }
        }

        void RemoveErroConnect()
        {
            mSocketConnect.RemoveAll((connect conn) =>
            {
                if (!conn.mConnectNormal)
                {
                    conn.Close();
                }
                return !conn.mConnectNormal;
            });

        }
    }
}


//客户端
namespace PackSocket
{
    public class SocketClient:IRev
    {
        connect mConnect = null;
        receiver mReceiver = null;
        RevData mRevData = null;
        public void Start(byte mark, string IP, int prort, IRev revProc)
        {
            IPAddress address = IPAddress.Parse(IP);
            IPEndPoint endpoint = new IPEndPoint(address, prort);
            Socket so = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);


            so.Connect(endpoint);
            byte[] by = new byte[1];
            by[0] = mark;
            so.Send(by, 1, 0);
            if (null == revProc) mReceiver = new receiver();
            else mReceiver = null;
            mConnect = new connect(so, null, revProc);

        }
        public bool Send(uint cmd, byte[] msg, uint msgSize)
        {
            if (null == mConnect) return false;
            return mConnect.Send(cmd, msg, msgSize);
        }
        public RevData ProcRev(ref uint cmdRev, ref int lenRev)
        {
            if (mRevData == null) return null;

            byte[] recvBytes = new byte[param.socketRevBufSize];
            int recBufPos = 0;
            int BufLen = 0;

            mRevData.recvBytes = null;
            while (true)
            {

                BufLen = mConnect.mConn.Receive(recvBytes, recBufPos, param.socketRevBufSize - recBufPos, 0);
                if (BufLen <= 0) return null;
                recBufPos += BufLen;

                if (recBufPos != param.socketRevBufSize) continue;
                recBufPos = 0;

                mReceiver.rev(recvBytes, this);
                if (null != mRevData.recvBytes) return mRevData;
            }
        }

        public void onRevData(RevData data)
        {
            mRevData.cmd = data.cmd;
            mRevData.recvBytes = data.recvBytes;
            mRevData.msglen = data.msglen;
        }
    }
}
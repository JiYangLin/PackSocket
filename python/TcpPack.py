import struct
from socket import *
import threading
from threading import RLock
import time

REV_LEN = 1024
MAX_LISTEN_NUM = 20
GUID_LEN = 32
NewMsg_Size_LEN = 4
MarkLen = 1
ConnMark_Def = 0xFF

# Server: void IServerRev(byte mark, byte[] recvBytes,   erroMsg );
# Client: void IClientRev(byte[] recvBytes,   erroMsg);
class newMsgMark:
    def __init__(self):
        super().__init__()
        self.NewMsgMarkStr = b"6DB62435625348A28426C876F6B04A88"
        self.NewMsgMarkStrEnd = b"CE1D1E7C9DBF4951BB414B6D9D2AA1C1"
    def isNewMsgMark(self,recvBuf):
        for i in range(GUID_LEN):
            if self.NewMsgMarkStr[i] != recvBuf[i]:
                 return False
        EndGDIDPos = REV_LEN - GUID_LEN
        for i in range(GUID_LEN):
            if self.NewMsgMarkStrEnd[i] != recvBuf[i + EndGDIDPos]:
                 return False
        for i in range(GUID_LEN + NewMsg_Size_LEN,EndGDIDPos):
            if 0 != recvBuf[i]:
                 return False
        return True
class Receiver:
    def __init__(self):
        self.__revSize = 0
        self.__msgSize = 0
        self.__newMsgMark =  newMsgMark()
        self.__InitBuf()
    def __InitBuf(self,bufsize=REV_LEN):
        self.__buf = bytearray()
        for i in range(bufsize):
            self.__buf.append(0)

    def rev(self,recvBuf, RevFinishFun):
        if self.__newMsgMark.isNewMsgMark(recvBuf):
            self.__revSize = 0
            self.__msgSize = struct.unpack('<I', recvBuf[GUID_LEN:GUID_LEN+NewMsg_Size_LEN])[0]
            if len(self.__buf) < self.__msgSize: self.__InitBuf(self.__msgSize)
            return
        
        CpySize = REV_LEN
        if self.__revSize + REV_LEN > self.__msgSize:
            CpySize = self.__msgSize - self.__revSize

        if 0 == self.__msgSize: return
        if len(self.__buf) < self.__revSize + CpySize: return

        for i in range(0,CpySize):self.__buf[self.__revSize+i] = recvBuf[i]
        self.__revSize = self.__revSize + CpySize
        
        if self.__revSize != self.__msgSize: return

        RevFinishFun(self.__buf, self.__revSize)
        self.__msgSize = 0
        self.__revSize = 0
class Sender:
    def __InitBuf(self):
        self.__SendBuf = bytearray()
        for i in range(REV_LEN):
            self.__SendBuf.append(0)
    def __init__(self):
        self.__InitBuf()
        self.__newMsgMark = newMsgMark()

    def __Send(self,sock):
        by = bytes(self.__SendBuf)
        sock.send(by)

    def SendMsg(self,sock, msg):
        msgSize = len(msg)
        self.__InitBuf()
        for i in range(0,GUID_LEN): self.__SendBuf[i] = self.__newMsgMark.NewMsgMarkStr[i]
        msgSizeBytes = struct.pack('<I',msgSize)
        for i in range(0,4): self.__SendBuf[i+GUID_LEN] = msgSizeBytes[i]
        pos = REV_LEN - GUID_LEN
        for i in range(0,GUID_LEN): self.__SendBuf[i + pos] = self.__newMsgMark.NewMsgMarkStrEnd[i]
        self.__Send(sock)

        hasSendSize = 0
        while True:
            self.__InitBuf()

            if msgSize <= REV_LEN:
                for i in range(0,msgSize): self.__SendBuf[i] = msg[i+hasSendSize]
                self.__Send(sock)
                return

            for i in range(0,REV_LEN): self.__SendBuf[i] = msg[i+hasSendSize]
            self.__Send(sock)

            hasSendSize = hasSendSize + REV_LEN
            msgSize = msgSize - REV_LEN
class MsgCtl:
    def __init__(self):
        self.__Receiver =  Receiver()
        self.__Sender =  Sender()
    def Rev(self,recvBuf, RevFinishFun):
        self.__Receiver.rev(recvBuf, RevFinishFun)
    def Send(self,sock, msg):
        self.__Sender.SendMsg(sock, msg)
class Connect:
    def __init__(self,_conn, _IServerRev, _IClientRev):
        self.__MsgCtl = MsgCtl()
        self.__ConnMark = ConnMark_Def
        self.__newMsgMark = newMsgMark()

        self.__IServerRev = _IServerRev
        self.__IClientRev = _IClientRev

        self.connectNormal = True
        self.__conn = _conn
        
        self.__ProcRevThr  = True
        self.__revThread = threading.Thread(target=self.__OnRev)  
        self.__revThread.setDaemon(True)  
        self.__revThread.start()
    def Close(self):
        if(None != self.__conn): self.__conn.close() 
    def Rev(self, msg,  msgSize,  erro):
        if None != msg : msg = msg[0:msgSize]
        if (None != self.__IServerRev): self.__IServerRev(self.__ConnMark, msg, erro)
        else: self.__IClientRev.Rev(msg, erro)
    def __OnRev(self):
        try:
            self.OnRev()
        except Exception as e:
            self.connectNormal = False
            self.Rev(None,0,e)
            print(self.__ConnMark)
            print(e)    

    def OnRev(self):
        recvBytes = bytearray()
        for i in range(REV_LEN):
            recvBytes.append(0)
        recBufPos = 0
        BufLen = 0

        if (self.__IServerRev != None):   
            markBytes =  self.__conn.recv(REV_LEN)
            self.__ConnMark = markBytes[0]
            print("connect mark :" + str(self.__ConnMark))

        while(True):
            if(self.__ProcRevThr == False): break 
            tempRevLen = REV_LEN - recBufPos
            tempRevBytes = self.__conn.recv(tempRevLen)            
            BufLen = len(tempRevBytes)
            for i in range(0,BufLen): recvBytes[recBufPos+i] = tempRevBytes[i]
            recBufPos += BufLen
            if recBufPos != REV_LEN: continue
            recBufPos = 0
            self.__MsgCtl.Rev(recvBytes,lambda revByte, revSize: self.Rev(revByte, revSize, ""))
    
    def Send(self,msg):
        try:
            self.__MsgCtl.Send(self.__conn, msg)
        except Exception as e:
            print(e)
            return False
        return True


class SocketServer:
    def __init__(self):
        self.__SocketConnect = []
        self.__lock = RLock()
    def Start(self,port,  _revProc):
        self.__revProc = _revProc
        self.__socketServer = socket(AF_INET, SOCK_STREAM)
        self.__socketServer .bind(("0.0.0.0",port))
        
        threadSocket = threading.Thread(target=self.__Accept)  
        threadSocket.setDaemon(True)  
        threadSocket.start()
    def __Accept(self):
        self.__socketServer .listen(MAX_LISTEN_NUM)
        while True:
            clientsock,clientaddress=self.__socketServer.accept()
            print('connect from:',clientaddress)
            self.__lock.acquire()
            self.__lock.acquire()
            self.__RemoveErroConnect()
            conn = Connect(clientsock, self.__revProc, None)
            self.__SocketConnect.append(conn)
            self.__lock.release()
            self.__lock.release()
    def __RemoveErroConnect(self):
        self.__SocketConnect = list(filter(lambda x: x.connectNormal == True,self.__SocketConnect))

    def SendToAllConn(self,msg):
        self.__SendToConn(msg)   
    def SendToMarkConn(self, mark, msg):
        self.__SendToConn(msg, mark)
    def __SendToConn(self, msg,  mark = ConnMark_Def):
        self.__lock.acquire()
        self.__lock.acquire()
        for conn in self.__SocketConnect:
            if conn.connectNormal == False: continue

            if mark != ConnMark_Def:
                if mark != conn.GetConnMark(): continue
            
            conn.Send(msg)
            
        self.__lock.release()
        self.__lock.release()

class SocketClient:
    def __init__(self):
        self.__Connect = None

    def Start(self,mark,  ip,  port, IClientRev):
        socketClient = socket(AF_INET, SOCK_STREAM)
        socketClient.connect((ip,port)) 
        socketClient.send(struct.pack('<B',mark))      
        self.__Connect = Connect(socketClient, None, IClientRev)

    def Send(self, msg):
        if None == self.__Connect: return False
        return self.__Connect.Send(msg)
from TcpPack import *

class Ser:
    def __IServerRev(self, mark,  recvBytes,  erroMsg):
        if (None == recvBytes): return
        print(mark)
        print(recvBytes)
    def Run(self):
        _SocketServer = SocketServer()
        _SocketServer.Start(1234,self.__IServerRev)
        print("=====Server====")
        while True:
            strData = input()
            by = strData.encode('utf-8') 
            print(by)
            _SocketServer.SendToAllConn(by)
            
ser = Ser()
ser.Run()
from TcpPack import *

class Ser:
    def __IServerRev(self, mark, cmd, recvBytes,revlen):
        if (None == recvBytes): return
        print(cmd)
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
            _SocketServer.SendToAllConn(10,by)
            
class Cli:
    def IClientRev(self, cmd,recvBytes,revlen):
        if (None == recvBytes): return
        print(cmd)
        print(recvBytes)
    def Run(self):
        print("input mark:")
        strMark = input()
        mark = int(strMark)

        _SocketClient = SocketClient()
        _SocketClient.Start(mark,"127.0.0.1",1234,self.IClientRev)
        print("=====client====")
        while True:
            strData = input()
            by = strData.encode('utf-8') 
            print(by)
            _SocketClient.Send(10,by)

print("is Server type(Y/N):")
typ = str(input())
if typ == "Y" or typ == "y":
    ser = Ser()
    ser.Run()
else:
    cl = Cli()
    cl.Run()

from TcpPack import *

class Cli:
    def IClientRev(self, recvBytes,  erroMsg):
        if (None == recvBytes): return
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
            _SocketClient.Send(by)
            
cl = Cli()
cl.Run()
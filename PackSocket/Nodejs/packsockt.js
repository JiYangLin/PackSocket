const net = require( 'net' );

const  SOCKET_REV_BUF_SIZE = 1024;
const  GUID_LEN =   16;
const  NewMsg_Size_LEN  =  4;
const  CMD_LEN   =   4;
const  MSG_HEAD = Buffer.from("4A6959616E674C69");
const  MSG_END = Buffer.from("414B6D9D2AA1C14A");

const   Pos_cmd = GUID_LEN + NewMsg_Size_LEN;
const   Pos_msg = Pos_cmd + CMD_LEN;
const   Pos_NewMsgMarkStrEnd = SOCKET_REV_BUF_SIZE - GUID_LEN;
const   newMsgMaxDataSpace = SOCKET_REV_BUF_SIZE - Pos_msg - GUID_LEN;


function Sender(mark)
{
    this.NewMsgBuf = Buffer.alloc(SOCKET_REV_BUF_SIZE);
    this.SendBuf = Buffer.alloc(SOCKET_REV_BUF_SIZE);
    this.socket = null;
    this.mark = mark;
    
    this.init = function(sock)
    {
        this.socket = sock;
        if(this.socket == null) return;

        MSG_HEAD.copy(this.NewMsgBuf, 0,0,GUID_LEN);
        MSG_END.copy(this.NewMsgBuf, Pos_NewMsgMarkStrEnd,0,GUID_LEN);

        const buf = Buffer.alloc(1);
        buf.writeUInt8(this.mark,0);
        this.socket.write(buf);
    }

    this.SendStr = function(cmd,str)
    {
        if(null == this.socket) return false;
        let datBuf = Buffer.from(str)
        return this.Send(cmd,datBuf)
    }
    this.Send = function(cmd,datBuf)
    {
        if(null == this.socket) return false;

        let msgSize = datBuf.length
        this.NewMsgBuf.writeInt32LE(msgSize,GUID_LEN);
        this.NewMsgBuf.writeInt32LE(cmd,Pos_cmd);
        if (msgSize <= newMsgMaxDataSpace)
        {
            datBuf.copy(this.NewMsgBuf, Pos_msg,0,msgSize);
            this.socket.write(this.NewMsgBuf);
            return true;
        }

        this.socket.write(this.NewMsgBuf);
        let hasSendSize = 0;
		while (true)
		{
			if (msgSize <= SOCKET_REV_BUF_SIZE)
			{
                datBuf.copy(this.SendBuf, 0,hasSendSize,hasSendSize +msgSize);
                this.socket.write(this.SendBuf);
                return true;
			}


            datBuf.copy(this.SendBuf, 0,hasSendSize,hasSendSize + SOCKET_REV_BUF_SIZE);
            this.socket.write(this.SendBuf);

			hasSendSize += SOCKET_REV_BUF_SIZE;
			msgSize -= SOCKET_REV_BUF_SIZE;
		}
    }
}

function Receiver(recverFinish)
{
    this.m_revSize = 0;
    this.m_msgSize = 0;
    this.m_cmd  = 0;
    this.m_recverFinish = recverFinish;
    this.m_buf =  Buffer.alloc(SOCKET_REV_BUF_SIZE)

    this.OnRev = function(recvBuf)
    {
        if(this.isNewMsgMark(recvBuf))
        {
            this.m_revSize = 0;
            this.m_msgSize = recvBuf.readUInt32LE(GUID_LEN);
            this.m_cmd = recvBuf.readUInt32LE(Pos_cmd);
            if (this.m_buf.length < this.m_msgSize) this.m_buf = Buffer.alloc(this.m_msgSize);

            if (this.m_msgSize <= newMsgMaxDataSpace)
            {
                recvBuf.copy(this.m_buf, 0,Pos_msg,Pos_msg +this.m_msgSize);
                this.m_recverFinish(this.m_cmd ,this.m_buf,this.m_msgSize)
            }
            return;
        }

        let CpySize = SOCKET_REV_BUF_SIZE;
        if (this.m_revSize + SOCKET_REV_BUF_SIZE > this.m_msgSize)  CpySize = this.m_msgSize - this.m_revSize;

        recvBuf.copy(this.m_buf, this.m_revSize,0,CpySize);
        this.m_revSize = this.m_revSize + CpySize;

        if (this.m_revSize != this.m_msgSize) return;
        this.m_recverFinish(this.m_cmd ,this.m_buf,this.m_msgSize)
    }
    this.isNewMsgMark = function(recvBuf)
    {
        if (!this.checkGUID(recvBuf, MSG_HEAD)) return false;
        if (!this.checkGUID(recvBuf, MSG_END, Pos_NewMsgMarkStrEnd)) return false;
        return true;
    }
    this.checkGUID = function(recvBuf,  guid, posStart = 0)
    {
        for (let i = 0; i < GUID_LEN; ++i)
        {
            if (guid[i] != recvBuf[i + posStart]) return false;
        }
        return true;
    }
}

function ClientSocket(mark,recver)
{
    this.socket = null;

    this.Receiver = new Receiver(recver);
    this.Sender = new Sender(mark);
    this.m_revbuf =  Buffer.alloc(SOCKET_REV_BUF_SIZE);
    this.m_revLen = 0;

    this.Start = function(Ip,  port)
    {
        this.socket =  new net.Socket();
        this.Sender.init(null);
        let _this  = this
        this.socket.connect( port,Ip,function(){
            console.log("connect suc");
            _this.Sender.init(_this.socket)
        });
        this.socket.on('data', function ( msgBuf ) {
            if(_this.m_revLen == 0)
            {
                if(msgBuf.length == SOCKET_REV_BUF_SIZE)
                {
                    _this.Receiver.OnRev(msgBuf); 
                    return;
                }
            }

            _this.RevBufProc(msgBuf);  
        }); 
        this.socket.on('error',function(){
            _this.socket = null;
        });
        this.socket.on('close',function(){
            console.log('close');
            _this.socket = null;
        });
    }
    this.RevBufProc = function(msgBuf)
    {
        let totoal = this.m_revLen + msgBuf.length;
        if(totoal == SOCKET_REV_BUF_SIZE)
        {
            msgBuf.copy(this.m_revbuf, this.m_revLen,0,msgBuf.length);
            this.Receiver.OnRev(this.m_revbuf); 
            this.m_revLen = 0;
        }
        else if(totoal < SOCKET_REV_BUF_SIZE)
        {
            msgBuf.copy(this.m_revbuf, this.m_revLen,0,msgBuf.length);
            this.m_revLen += msgBuf.length;
        }
        else
        {
            let copyedlen = 0;
            let needCopylen = 0;
            while(true)
            {
                needCopylen = msgBuf.length - copyedlen;
                if(needCopylen <= 0) return;

                totoal = needCopylen + this.m_revLen;
                if(totoal == SOCKET_REV_BUF_SIZE )
                {
                    msgBuf.copy(this.m_revbuf, this.m_revLen,copyedlen,copyedlen + needCopylen);
                    this.Receiver.OnRev(this.m_revbuf); 
                    this.m_revLen = 0;
                    copyedlen += needCopylen;
                }
                else if(totoal < SOCKET_REV_BUF_SIZE)
                {
                    msgBuf.copy(this.m_revbuf, this.m_revLen,copyedlen,copyedlen + needCopylen);
                    this.m_revLen += needCopylen;
                    copyedlen += needCopylen;
                }
                else
                {
                    let len = SOCKET_REV_BUF_SIZE - this.m_revLen;
                    msgBuf.copy(this.m_revbuf, this.m_revLen,copyedlen,copyedlen + len);
                    this.Receiver.OnRev(this.m_revbuf); 
                    this.m_revLen = 0;
                    copyedlen += len;
                }
            }
        }
    }
}


let cl = new ClientSocket(0,function(cmd,datBuf,datlen){
   let str = datBuf.toString('utf8',0,datlen)
   console.log(str);
});

cl.Start('127.0.0.1',1234);
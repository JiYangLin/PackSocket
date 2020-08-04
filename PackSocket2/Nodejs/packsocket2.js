const net = require( 'net' );

function User()
{
	this.answerMode = 0;// uint8_t   answerMode;
	this.id = 0;// uint8_t   id;
	this.name = 0;// char   name[32];
	this.password = 0;// char   password[32];
}
const UserLen = 66;

function MsgHead()
{
	this.cmd;// uint32_t cmd;
	this.code;// uint32_t code;
	this.msgLen;// uint32_t msgLen;
};
const  MsgHeadLen = 12;



function DataRev(recver){
    this.m_recver = recver;
    this.m_waitHead = true;

    this.m_head = new MsgHead();
    this.m_bufheadRevSize = 0;
    this.m_bufHead = Buffer.alloc(MsgHeadLen);

    this.m_bufRevSize = 0;
    this.m_buf = Buffer.alloc(1024);

    this.OnRev = function(msg)
    {
        hasReadLen = 0;
        while(true)
        {
            if(this.m_waitHead) hasReadLen = this.ProcGetHead(msg,hasReadLen);
            else hasReadLen = this.ProcGetData(msg,hasReadLen);
            if(hasReadLen == msg.length) return;
        }
    }
    this.ProcGetHead = function(msg,hasReadLen)
    {
        noReadLen = msg.length - hasReadLen;
        bufheadNeedRevLen = MsgHeadLen -  this.m_bufheadRevSize;
        if(bufheadNeedRevLen <= noReadLen)
        {
            endpos = hasReadLen + bufheadNeedRevLen;
            msg.copy(this.m_bufHead, this.m_bufheadRevSize,hasReadLen,endpos);
            this.GetHead();
            return endpos;
        }
        else
        {
            msg.copy(this.m_bufHead, this.m_bufheadRevSize,hasReadLen,msg.length);
            return msg.length;
        }
    }
    this.GetHead = function(){
        this.m_head.cmd = this.m_bufHead.readUInt32LE();
        this.m_head.code = this.m_bufHead.readUInt32LE(4);
        this.m_head.msgLen = this.m_bufHead.readUInt32LE(8);

        if(this.m_head.msgLen  == 0)  CallBackData(null);
        else
        {   
            if(this.m_buf.length < this.m_head.length) this.m_buf =  Buffer.alloc(this.m_head.length);
            this.m_waitHead = false;
        }
    }
    this.ProcGetData = function(msg,hasReadLen)
    {
        noReadLen = msg.length - hasReadLen;
        bufRevNeedSize = this.m_head.msgLen -  this.m_bufRevSize;
        if(bufRevNeedSize <= noReadLen)
        {
            endpos = hasReadLen + bufRevNeedSize;
            msg.copy(this.m_buf, this.m_bufRevSize,hasReadLen,endpos);
            CallBackData(this.m_buf);
            return endpos;
        }
        else
        {
            msg.copy(this.m_buf, this.m_bufRevSize,hasReadLen,msg.length);
            return msg.length;
        }
    }
    this.CallBackData(data)
    {
        this.m_recver(this.m_head,data);
        this.m_bufRevSize = 0;
        this.m_bufheadRevSize = 0;
        this.m_waitHead = true;
    }
}


function ClientSocket()
{
    this.socket = null;
    this.revObj = null;
    this.Start = function(user, Ip,  port, recver)
    {
        this.revObj = new DataRev(recver);
        this.socket =  new net.Socket();
        let _this  = this
        this.socket.connect( port,Ip,function(){
            _this.SendUser(user);
        });
        this.socket.on( 'data', function ( msg ) {
            _this.revObj.OnRev(msg);  
        });
          
        this.socket.on( 'error', function ( error ) {
            _this.socket = null;
          });
         this.socket.on('close',function(){
            console.log('close');
            _this.socket = null;
        });
    }

    this.SendUser = function(user)
    {
        const buf = Buffer.alloc(UserLen);
        buf.writeUInt8(user.answerMode,0);
        buf.writeUInt8(user.id,1);
        buf.write(user.name,2);
        buf.write(user.password,34);
        this.Send(buf);
    }

    this.Send = function(bf)
    {
        if(null == this.socket) return false;
        this.socket.write(bf);
        return true;
    }
}



let cl = new ClientSocket();
user = new User();
user.name = 'jyl';
user.password = 'jyl';
cl.Start(user,'172.16.80.72',1234,function(head,data){
    cmd = head.cmd;
    console.log(data.toString('utf8',0,head.msgLen))
});
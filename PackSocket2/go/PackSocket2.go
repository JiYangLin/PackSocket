package main

import (
	"bytes"
	"encoding/binary"
	"net"
	"fmt"
	"time"
)


func uint32ToBytes(data uint32) []byte {
	bytebuf := bytes.NewBuffer([]byte{})
	binary.Write(bytebuf, binary.LittleEndian, data)
	return bytebuf.Bytes()
}
func bytesToUInt32(bys []byte, startpos int) uint32 {
	bytebuff := bytes.NewBuffer(bys[startpos : startpos+4])
	var data uint32
	binary.Read(bytebuff, binary.LittleEndian, &data)
	return data
}
func float32ToBytes(data float32) []byte {
	bytebuf := bytes.NewBuffer([]byte{})
	binary.Write(bytebuf, binary.LittleEndian, data)
	return bytebuf.Bytes()
}
func bytesTofloat32(bys []byte, startpos int) float32 {
	bytebuff := bytes.NewBuffer(bys[startpos : startpos+4])
	var data float32
	binary.Read(bytebuff, binary.LittleEndian, &data)
	return data
}

//User user
type User struct {
	answerMode uint8
	id         uint8
	name       []byte //[32]
	password   []byte //[32]
}
func (_this *User) initUser(name string,password string) {
	_this.answerMode = 1
	_this.id = 0
	_this.name = make([]byte,32)
	_this.password = make([]byte,32)
	var temp = []byte(name)
	var si = len(temp)
	copy(_this.name[0:si],temp[0:si])
	temp = []byte(password)
	si = len(temp)
	copy(_this.password[0:si],temp[0:si])
}

//MsgHead msghead
type MsgHead struct {
	cmd       uint32
	code      uint32
	BigMsgLen uint32
	headMsg   []byte //[1012]
}
func (_this *MsgHead) initMsgHead() {
	_this.cmd = 0
	_this.code = 0
	_this.BigMsgLen = 0
	_this.headMsg = make([]byte,1012)
}
func (_this *MsgHead)userToMsgHead(user *User){
	_this.headMsg[0] = user.answerMode
	_this.headMsg[1] = user.id
	var end = 2+32
	copy(_this.headMsg[2:end], user.name[0:32])
	i := end
	end = i + 32
	copy(_this.headMsg[i:end], user.password[0:32])
}
func (_this *MsgHead)msgHeadToBytes(byMsgHead []byte){
	bycmd := uint32ToBytes(_this.cmd);
	copy(byMsgHead[0:4],bycmd[0:4]);

	bycode := uint32ToBytes(_this.code);
	copy(byMsgHead[4:8],bycode[0:4]);

	byBigMsgLen := uint32ToBytes(_this.BigMsgLen);
	copy(byMsgHead[8:12],byBigMsgLen[0:4]);
	
	copy(byMsgHead[12:1024],_this.headMsg[0:]);
}
func (_this *MsgHead)bytesToMsgHead(byMsgHead []byte){
	_this.cmd = bytesToUInt32(byMsgHead,0)
	_this.code = bytesToUInt32(byMsgHead,4)
	_this.BigMsgLen = bytesToUInt32(byMsgHead,8)

	copy(_this.headMsg[0:1012],byMsgHead[12:1024])
}


const (
	userLen    = 66
	msgHeadLen = 1024

	pS2ErrDEFUserCheckSuc = 0
	pS2ErrDEFUserCheckErr = 1
	pS2ErrDEFSocketSendErr = 2
	pS2ErrDEFSocketRevErr =3
)

//PackSocketMSG packSocketMSG
type PackSocketMSG struct {
	head MsgHead
	data []byte
}
//GetData getdata
func (_this *PackSocketMSG)GetData() []byte{
	if _this.head.BigMsgLen == 0{
		len := bytesToUInt32(_this.head.headMsg,0)
		return _this.head.headMsg[4:4+len]
	}
	return  _this.data[0:_this.head.BigMsgLen]
}



//IClientRecver iClientRecver
type IClientRecver interface {
	onDisConnected(errCode uint8)
}


type socketConn struct {
	mConn           net.Conn
	mIClientRecver IClientRecver
	mbConnect bool
	mMsgRevData PackSocketMSG
	mbyHeadBuf []byte
	mbyDataBuf []byte
}
func (_this *socketConn)Release(){
	_this.mbConnect = false
	if(nil != _this.mConn){
		_this.mConn.Close()
	}
}
func (_this *socketConn)InitByClient(Conn net.Conn, pIClientRecver IClientRecver, user *User)bool{
	_this.mConn = Conn
	_this.mIClientRecver = pIClientRecver
	_this.mbConnect = true
	_this.mbyHeadBuf = make([]byte,msgHeadLen)

	_this.mMsgRevData.head.initMsgHead()
	_this.mMsgRevData.head.userToMsgHead(user)
	if (nil == _this.SendAnswer(&_this.mMsgRevData)){
		return false
	}
	if (_this.mMsgRevData.head.cmd != pS2ErrDEFUserCheckSuc){
		_this.OnDisConnected(pS2ErrDEFUserCheckErr)
		return false;
	}
	return true;
}
func (_this *socketConn)Send(msg *PackSocketMSG)bool{
	if (!_this.mbConnect){
		return false
	}
	msg.head.msgHeadToBytes(_this.mbyHeadBuf);
	ret := _this.SendData(_this.mbyHeadBuf)
	if (msg.head.BigMsgLen <= 0){
		return ret
	}
	if (!ret){
		return false
	}
	return _this.SendData(msg.data)
}
func (_this *socketConn)SendAnswer(msg *PackSocketMSG)*PackSocketMSG{
	if _this.Send(msg) == false{
		return nil
	} 
	return _this.FetchData()
}
func (_this *socketConn)SendData(data []byte)bool{
	hasSendSize := 0
	dataLen := len(data)
	for{
		sendLen := _this.sendmsgFun(data,hasSendSize,dataLen-hasSendSize)
		if sendLen <= 0 {
			_this.OnDisConnected(pS2ErrDEFSocketSendErr)
			return false
		}
		hasSendSize += sendLen
		if hasSendSize == dataLen{
			return true
		}
	}
}
func (_this *socketConn) sendmsgFun(SendBuf []byte,startpos int,endPos int) int {
	n, err := _this.mConn.Write(SendBuf[startpos:endPos])
	if err != nil {
		fmt.Println("sendmsg error:", err)
	}
	return n
}
func (_this *socketConn)OnDisConnected(errCode uint8){
	_this.mbConnect = false
	_this.mIClientRecver.onDisConnected(errCode);
	_this.Release();
}
func (_this *socketConn)FetchData()*PackSocketMSG{
	if !_this.RevData(_this.mbyHeadBuf,msgHeadLen){
		return nil
	}

	_this.mMsgRevData.head.bytesToMsgHead(_this.mbyHeadBuf)
	if _this.mMsgRevData.head.BigMsgLen > 0{
		datalen :=  int(_this.mMsgRevData.head.BigMsgLen)
		if  len(_this.mbyDataBuf) < datalen{
			_this.mbyDataBuf = make([]byte,datalen)
		}	
		_this.RevData(_this.mbyDataBuf,datalen)
		_this.mMsgRevData.data = _this.mbyDataBuf
	}

	return &_this.mMsgRevData
}
func (_this *socketConn)RevData(retby []byte,revTotoalLen int)bool{
	if revTotoalLen <= 0{
		return true
	}

	recBufPos := 0
	for{
		revlen := _this.revmsg(retby,recBufPos, revTotoalLen - recBufPos)
		recBufPos += revlen
		if revlen <= 0{
			_this.OnDisConnected(pS2ErrDEFSocketRevErr);
			return false
		}
		if recBufPos == revTotoalLen{
			return true
		}
	}

	return false
}
func (_this *socketConn) revmsg(recvBuf []byte,startpos int,endPos int) int {
	revlen, err := _this.mConn.Read(recvBuf[startpos:endPos])
	if err != nil {
		fmt.Println("revmsg error:", err)
		return 0
	}
	return revlen
}


//ClientSocket cl
type ClientSocket struct{
	mConnect *socketConn
}
//Start Start Client
func (_this *ClientSocket)Start(ip string, port string, pIClientRecver IClientRecver, user *User)bool{
	if nil == _this.mConnect{
		_this.mConnect = new(socketConn)
	}
	
	if  _this.mConnect.mbConnect{
		return true
	}

	conn, err := net.DialTimeout("tcp", ip+":"+port, 2*time.Second)
	if err != nil {
		fmt.Println("connect error:", err)
		return false
	}

	if !_this.mConnect.InitByClient(conn, pIClientRecver, user){
		return false
	} 

	return true
}
//SendAnswer SendAnswerfun
func (_this *ClientSocket)SendAnswer(msg *PackSocketMSG)*PackSocketMSG{
	if  !_this.mConnect.mbConnect{
		return nil
	}
	return _this.mConnect.SendAnswer(msg);
}



type clientTest struct {

}
func (_this *clientTest)onDisConnected(errCode uint8){

}
//TestRun test
func TestRun(){
	ii := clientTest{}
	user := User{}
	user.initUser("jyl","jyl")
	cl := ClientSocket{nil}
	cl.Start("127.0.0.1","1234",&ii,&user)
}
package packsocket

import (
	"bytes"
	"container/list"
	"encoding/binary"
	"fmt"
	"net"
	"sync"
	"time"
)

func uint32ToBytes(data uint32) []byte {
	bytebuf := bytes.NewBuffer([]byte{})
	binary.Write(bytebuf, binary.LittleEndian, data)
	return bytebuf.Bytes()
}
func bytesToUInt32(bys []byte, startpos int) int {
	bytebuff := bytes.NewBuffer(bys[startpos : startpos+4])
	var data uint32
	binary.Read(bytebuff, binary.LittleEndian, &data)
	return int(data)
}

const (
	socketRevBufSize = 1024

	guidLEN          = 16
	newMsgMarkStr    = "4A6959616E674C696E6DB62435625348"
	newMsgMarkStrEnd = "414B6D9D2AA1C14A6959616E674C696E"
	newMsgSizeLEN    = 4
	cmdLen           = 4

	posCmd                   = 20
	posMsg                   = 24
	posNewMsgMarkStrEnd      = 1008
	newMsgMaxDataSpace       = 984
	connMarkDef         byte = 255
)

//RevData revdata
type RevData struct {
	mark      byte
	cmd       uint32
	recvBytes []byte
	msglen    int
}

func (_this *RevData) init() {
	_this.mark = 0
	_this.cmd = 0
	_this.recvBytes = make([]byte, socketRevBufSize)
	_this.msglen = 0
}

//IRev imp
type IRev interface {
	onRec(data *RevData)
}

/////////receiver
type receiver struct {
	mRevSize       int
	mRevData       RevData
	mNewMsgMark    []byte
	mNewMsgMarkEnd []byte
}

func (_this *receiver) init() {
	_this.mRevSize = 0
	_this.mRevData.init()
	_this.mNewMsgMark = []byte(newMsgMarkStr)
	_this.mNewMsgMarkEnd = []byte(newMsgMarkStrEnd)
}
func (_this *receiver) rev(recvBuf []byte, Fun IRev) {
	if _this.isNewMsgMark(recvBuf) {
		_this.mRevSize = 0
		_this.mRevData.msglen = bytesToUInt32(recvBuf, guidLEN)
		_this.mRevData.cmd = uint32(bytesToUInt32(recvBuf, posCmd))
		if len(_this.mRevData.recvBytes) < int(_this.mRevData.msglen) {
			_this.mRevData.recvBytes = make([]byte, _this.mRevData.msglen)
		}

		if _this.mRevData.msglen <= newMsgMaxDataSpace {
			copy(_this.mRevData.recvBytes[0:_this.mRevData.msglen], recvBuf[posMsg:posMsg+_this.mRevData.msglen])
			_this.msgFinish(Fun)
		}
		return
	}

	CpySize := socketRevBufSize
	if _this.mRevSize+socketRevBufSize > _this.mRevData.msglen {
		CpySize = _this.mRevData.msglen - _this.mRevSize
	}

	copy(_this.mRevData.recvBytes[_this.mRevSize:], recvBuf[0:CpySize])
	_this.mRevSize = _this.mRevSize + CpySize

	if _this.mRevSize == _this.mRevData.msglen {
		_this.msgFinish(Fun)
	}
}
func (_this *receiver) isNewMsgMark(recvBuf []byte) bool {
	if !_this.checkGUID(recvBuf, _this.mNewMsgMark, 0) {
		return false
	}
	if !_this.checkGUID(recvBuf, _this.mNewMsgMarkEnd, posNewMsgMarkStrEnd) {
		return false
	}
	return true
}
func (_this *receiver) msgFinish(Fun IRev) {
	Fun.onRec(&_this.mRevData)
	_this.mRevData.msglen = 0
	_this.mRevSize = 0
}
func (_this *receiver) checkGUID(recvBuf []byte, guid []byte, posStart int) bool {
	for i := 0; i < guidLEN; i++ {
		if guid[i] != recvBuf[i+posStart] {
			return false
		}
	}
	return true
}

//////

///////////sender
type sender struct {
	mNewMsgMark    []byte
	mNewMsgMarkEnd []byte
}

func (_this *sender) init() {
	_this.mNewMsgMark = []byte(newMsgMarkStr)
	_this.mNewMsgMarkEnd = []byte(newMsgMarkStrEnd)
}
func (_this *sender) sendmsgFun(sock net.Conn, SendBuf []byte) bool {
	n, err := sock.Write(SendBuf)
	if err != nil {
		fmt.Println("sendmsg error:", err)
	}
	return socketRevBufSize == n
}
func (_this *sender) SendMsg(sock net.Conn, cmd uint32, msg []byte, msgSize int) bool {
	SendBuf := make([]byte, socketRevBufSize)

	copy(SendBuf[0:guidLEN], _this.mNewMsgMark[0:guidLEN])
	copy(SendBuf[guidLEN:guidLEN+newMsgSizeLEN], uint32ToBytes(uint32(msgSize))[0:newMsgSizeLEN])
	copy(SendBuf[posCmd:posCmd+cmdLen], uint32ToBytes(cmd)[0:cmdLen])
	copy(SendBuf[posNewMsgMarkStrEnd:posNewMsgMarkStrEnd+guidLEN], _this.mNewMsgMarkEnd[0:guidLEN])
	if msgSize <= newMsgMaxDataSpace {
		copy(SendBuf[posMsg:posMsg+msgSize], msg[0:msgSize])
		return _this.sendmsgFun(sock, SendBuf)
	}
	suc := _this.sendmsgFun(sock, SendBuf)
	if !suc {
		return false
	}

	hasSendSize := 0
	for {
		if msgSize <= socketRevBufSize {
			copy(SendBuf[0:msgSize], msg[hasSendSize:hasSendSize+msgSize])
			return _this.sendmsgFun(sock, SendBuf)
		}

		copy(SendBuf[0:socketRevBufSize], msg[hasSendSize:hasSendSize+socketRevBufSize])
		suc = _this.sendmsgFun(sock, SendBuf)
		if !suc {
			return false
		}

		hasSendSize += socketRevBufSize
		msgSize -= socketRevBufSize
	}
}

////////socketConn
type socketConn struct {
	mConn           net.Conn
	mPIServerRecver IRev
	mPIClientRecver IRev

	mSender   sender
	mReceiver receiver

	mBConnect bool

	mConnMark byte
}

func (_this *socketConn) onRec(data *RevData) {
	if nil != _this.mPIServerRecver {
		if nil != data {
			data.mark = _this.mConnMark
		}
		_this.mPIServerRecver.onRec(data)
	} else {
		_this.mPIClientRecver.onRec(data)
	}
}
func (_this *socketConn) revmsg(recvBuf []byte) int {
	revlen, err := _this.mConn.Read(recvBuf)
	if err != nil {
		fmt.Println("revmsg error:", err)
		return 0
	}
	return revlen
}
func (_this *socketConn) init(Conn net.Conn, pIServerRecver IRev, pIClientRecver IRev) {
	_this.mConn = Conn
	_this.mBConnect = true
	_this.mConnMark = connMarkDef
	_this.mPIServerRecver = pIServerRecver
	_this.mPIClientRecver = pIClientRecver
	_this.mSender.init()
	_this.mReceiver.init()
	if nil == pIServerRecver {
		if nil == pIClientRecver {
			return
		}
		go func() {
			recvBuf := make([]byte, socketRevBufSize)
			recBufPos := 0
			BufLen := 0

			if _this.mPIServerRecver != nil {
				BufLen = _this.revmsg(recvBuf[:1])
				if _this.recvErro(BufLen <= 0) {
					return
				}
				_this.mConnMark = recvBuf[0]
			}

			var Fun IRev = _this
			for {

				BufLen = _this.revmsg(recvBuf[recBufPos:])
				recBufPos += BufLen

				if _this.recvErro(BufLen <= 0) {
					return
				}

				if recBufPos != socketRevBufSize {
					continue
				}
				recBufPos = 0

				_this.mReceiver.rev(recvBuf, Fun)
			}
		}()
	}
}

func (_this *socketConn) recvErro(bErro bool) bool {
	if !bErro {
		return false
	}
	_this.mBConnect = false
	_this.onRec(nil)
	_this.mConn.Close()
	return true
}
func (_this *socketConn) Send(cmd uint32, msg []byte, msgSize int) bool {
	if !_this.mBConnect {
		return false
	}
	return _this.mSender.SendMsg(_this.mConn, cmd, msg, msgSize)
}

////////

//ServerSocket def
type ServerSocket struct {
	mSockSrv        net.Listener
	mPISocketRecver IRev

	mConnect *list.List
	mLock    *sync.Mutex
}

//Start socket
func (_this *ServerSocket) Start(port string, pISocketRecver IRev) bool {
	_this.mLock = new(sync.Mutex)

	_this.mConnect = list.New()

	_this.mPISocketRecver = pISocketRecver

	l, err := net.Listen("tcp", ":"+port)
	if err != nil {
		fmt.Println("listen error:", err)
		return false
	}

	go func() {
		for {
			conn, err := l.Accept()
			if err != nil {
				fmt.Println("accept error:", err)
				continue
			}

			_this.mLock.Lock()
			_this.removeInvalid()
			connect := new(socketConn)
			_this.mConnect.PushBack(connect)
			connect.init(conn, _this.mPISocketRecver, nil)
			_this.mLock.Unlock()
		}
	}()

	return true
}

//SendToAllConn all
func (_this *ServerSocket) SendToAllConn(cmd uint32, msg []byte, msgSize int) bool {
	return _this.sendToConn(cmd, msg, msgSize, connMarkDef)
}

//SendToMarkConn single
func (_this *ServerSocket) SendToMarkConn(mark byte, cmd uint32, msg []byte, msgSize int) bool {
	return _this.sendToConn(cmd, msg, msgSize, mark)
}
func (_this *ServerSocket) removeInvalid() {
	temp := list.New()
	for i := _this.mConnect.Front(); i != nil; i = i.Next() {
		var conn *socketConn = i.Value.(*socketConn)
		if conn.mBConnect {
			temp.PushBack(i)
		}
	}
	_this.mConnect = temp
}
func (_this *ServerSocket) sendToConn(cmd uint32, msg []byte, msgSize int, mark byte) bool {
	sendSuc := false
	_this.mLock.Lock()

	for i := _this.mConnect.Front(); i != nil; i = i.Next() {
		var conn *socketConn = i.Value.(*socketConn)
		if !conn.mBConnect {
			continue
		}

		if mark != connMarkDef {
			if mark != conn.mConnMark {
				continue
			}
		}

		sendSuc = conn.Send(cmd, msg, msgSize)
	}

	_this.mLock.Unlock()
	return sendSuc
}

//////

//ClientSocket def
type ClientSocket struct {
	mConnect  *socketConn
	mReceiver receiver
	mRevData  RevData
}

func (_this *ClientSocket) init() {
	_this.mConnect = nil
	_this.mReceiver.init()
}

//Start startClinet
func (_this *ClientSocket) Start(mark byte, IP string, port string, recver IRev) bool {
	conn, err := net.DialTimeout("tcp", IP+":"+port, 2*time.Second)
	if err != nil {
		fmt.Println("connect error:", err)
		return false
	}

	data := make([]byte, 1)
	data[0] = mark
	n, erro := conn.Write(data)
	if erro != nil {
		fmt.Println("connect error:", erro)
		return false
	}
	if n != 1 {
		return false
	}

	_this.mConnect = new(socketConn)
	_this.mConnect.init(conn, nil, recver)
	return true
}

//Send ClinetSendMsg
func (_this *ClientSocket) Send(cmd uint32, msg []byte, msgSize int) bool {
	if nil == _this.mConnect {
		return false
	}
	return _this.mConnect.Send(cmd, msg, msgSize)
}

//ProcRev callafterSend
func (_this *ClientSocket) ProcRev() *RevData {
	_this.mRevData.recvBytes = nil

	recvBuf := make([]byte, socketRevBufSize)
	recBufPos := 0
	BufLen := 0

	var Fun IRev = _this
	for {
		BufLen = _this.revmsg(recvBuf[recBufPos:])
		recBufPos += BufLen

		if BufLen <= 0 {
			return nil
		}

		if recBufPos != socketRevBufSize {
			continue
		}
		recBufPos = 0

		_this.mReceiver.rev(recvBuf, Fun)

		if _this.mRevData.recvBytes != nil {
			return &_this.mRevData
		}
	}
}
func (_this *ClientSocket) revmsg(recvBuf []byte) int {
	revlen, err := _this.mConnect.mConn.Read(recvBuf)
	if err != nil {
		fmt.Println("revmsg error:", err)
		return 0
	}
	return revlen
}

func (_this *ClientSocket) onRec(data *RevData) {
	_this.mRevData.recvBytes = data.recvBytes
	_this.mRevData.cmd = data.cmd
	_this.mRevData.msglen = data.msglen
}

////////

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
	SOCKET_REV_BUF_SIZE = 1024

	GUID_LEN         = 16
	NewMsgMarkStr    = "6DB62435625348A28426C876F6B04A88"
	NewMsgMarkStrEnd = "CE1D1E7C9DBF4951BB414B6D9D2AA1C1"
	NewMsg_Size_LEN  = 4
	CMD_LEN          = 4

	Pos_cmd                   = 20
	Pos_msg                   = 24
	Pos_NewMsgMarkStrEnd      = 1008
	newMsgMaxDataSpace        = 984
	ConnMark_Def         byte = 255
)

//IServerRecver serverimp
type IServerRecver interface {
	OnRec(mark byte, cmd uint32, recvBuf []byte, BufLen int)
}

//IClientRecver clientimp
type IClientRecver interface {
	OnRec(cmd uint32, recvBuf []byte, BufLen int)
}

type iRevFinish interface {
	OnRevFinish(cmd uint32, recvBuf []byte, BufLen int)
}

/////////receiver
type receiver struct {
	m_revSize       int
	m_msgSize       int
	m_cmd           uint32
	m_buf           []byte
	m_NewMsgMark    []byte
	m_NewMsgMarkEnd []byte
}

func (_this *receiver) init() {
	_this.m_revSize = 0
	_this.m_msgSize = 0
	_this.m_cmd = 0
	_this.m_buf = make([]byte, SOCKET_REV_BUF_SIZE)
	_this.m_NewMsgMark = []byte(NewMsgMarkStr)
	_this.m_NewMsgMarkEnd = []byte(NewMsgMarkStrEnd)
}
func (_this *receiver) rev(recvBuf []byte, Fun iRevFinish) {
	if _this.isNewMsgMark(recvBuf) {
		_this.m_revSize = 0
		_this.m_msgSize = bytesToUInt32(recvBuf, GUID_LEN)
		_this.m_cmd = uint32(bytesToUInt32(recvBuf, Pos_cmd))
		if len(_this.m_buf) < int(_this.m_msgSize) {
			_this.m_buf = make([]byte, _this.m_msgSize)
		}

		if _this.m_msgSize <= newMsgMaxDataSpace {
			copy(_this.m_buf[0:_this.m_msgSize], recvBuf[Pos_msg:Pos_msg+_this.m_msgSize])
			_this.msgFinish(Fun)
		}
		return
	}

	CpySize := SOCKET_REV_BUF_SIZE
	if _this.m_revSize+SOCKET_REV_BUF_SIZE > _this.m_msgSize {
		CpySize = _this.m_msgSize - _this.m_revSize
	}

	copy(_this.m_buf[_this.m_revSize:], recvBuf[0:CpySize])
	_this.m_revSize = _this.m_revSize + CpySize

	if _this.m_revSize == _this.m_msgSize {
		_this.msgFinish(Fun)
	}
}
func (_this *receiver) isNewMsgMark(recvBuf []byte) bool {
	if !_this.checkGUID(recvBuf, _this.m_NewMsgMark, 0) {
		return false
	}
	if !_this.checkGUID(recvBuf, _this.m_NewMsgMarkEnd, Pos_NewMsgMarkStrEnd) {
		return false
	}
	return true
}
func (_this *receiver) msgFinish(Fun iRevFinish) {
	Fun.OnRevFinish(_this.m_cmd, _this.m_buf, _this.m_msgSize)
	_this.m_msgSize = 0
	_this.m_revSize = 0
}
func (_this *receiver) checkGUID(recvBuf []byte, guid []byte, posStart int) bool {
	for i := 0; i < GUID_LEN; i++ {
		if guid[i] != recvBuf[i+posStart] {
			return false
		}
	}
	return true
}

//////

///////////sender
type sender struct {
	m_NewMsgMark    []byte
	m_NewMsgMarkEnd []byte
}

func (_this *sender) init() {
	_this.m_NewMsgMark = []byte(NewMsgMarkStr)
	_this.m_NewMsgMarkEnd = []byte(NewMsgMarkStrEnd)
}
func (_this *sender) sendmsgFun(sock net.Conn, SendBuf []byte) bool {
	n, err := sock.Write(SendBuf)
	if err != nil {
		fmt.Println("sendmsg error:", err)
	}
	return SOCKET_REV_BUF_SIZE == n
}
func (_this *sender) SendMsg(sock net.Conn, cmd uint32, msg []byte, msgSize int) bool {
	SendBuf := make([]byte, SOCKET_REV_BUF_SIZE)

	copy(SendBuf[0:GUID_LEN], _this.m_NewMsgMark[0:GUID_LEN])
	copy(SendBuf[GUID_LEN:GUID_LEN+NewMsg_Size_LEN], uint32ToBytes(uint32(msgSize))[0:NewMsg_Size_LEN])
	copy(SendBuf[Pos_cmd:Pos_cmd+CMD_LEN], uint32ToBytes(cmd)[0:CMD_LEN])
	copy(SendBuf[Pos_NewMsgMarkStrEnd:Pos_NewMsgMarkStrEnd+GUID_LEN], _this.m_NewMsgMarkEnd[0:GUID_LEN])
	if msgSize <= newMsgMaxDataSpace {
		copy(SendBuf[Pos_msg:Pos_msg+msgSize], msg[0:msgSize])
		return _this.sendmsgFun(sock, SendBuf)
	}
	suc := _this.sendmsgFun(sock, SendBuf)
	if !suc {
		return false
	}

	hasSendSize := 0
	for {
		if msgSize <= SOCKET_REV_BUF_SIZE {
			copy(SendBuf[0:msgSize], msg[hasSendSize:hasSendSize+msgSize])
			return _this.sendmsgFun(sock, SendBuf)
		}

		copy(SendBuf[0:SOCKET_REV_BUF_SIZE], msg[hasSendSize:hasSendSize+SOCKET_REV_BUF_SIZE])
		suc = _this.sendmsgFun(sock, SendBuf)
		if !suc {
			return false
		}

		hasSendSize += SOCKET_REV_BUF_SIZE
		msgSize -= SOCKET_REV_BUF_SIZE
	}
}

////////socketConn
type socketConn struct {
	m_Conn           net.Conn
	m_pIServerRecver *IServerRecver
	m_pIClientRecver *IClientRecver

	m_sender   sender
	m_receiver receiver

	m_bConnect bool

	m_ConnMark byte
}

func (_this *socketConn) OnRevFinish(cmd uint32, recvBuf []byte, BufLen int) {
	if nil != _this.m_pIServerRecver {
		(*_this.m_pIServerRecver).OnRec(_this.m_ConnMark, cmd, recvBuf, BufLen)
	} else {
		(*_this.m_pIClientRecver).OnRec(cmd, recvBuf, BufLen)
	}
}
func (_this *socketConn) revmsg(recvBuf []byte) int {
	revlen, err := _this.m_Conn.Read(recvBuf)
	if err != nil {
		fmt.Println("revmsg error:", err)
		return 0
	}
	return revlen
}
func (_this *socketConn) init(Conn net.Conn, pIServerRecver *IServerRecver, pIClientRecver *IClientRecver) {
	_this.m_Conn = Conn
	_this.m_bConnect = true
	_this.m_ConnMark = ConnMark_Def
	_this.m_pIServerRecver = pIServerRecver
	_this.m_pIClientRecver = pIClientRecver
	_this.m_sender.init()
	_this.m_receiver.init()
	go func() {
		recvBuf := make([]byte, SOCKET_REV_BUF_SIZE)
		recBufPos := 0
		BufLen := 0

		if _this.m_pIServerRecver != nil {
			BufLen = _this.revmsg(recvBuf[:1])
			if _this.recvErro(BufLen <= 0) {
				return
			}
			_this.m_ConnMark = recvBuf[0]
		}

		var Fun iRevFinish = _this
		for {

			BufLen = _this.revmsg(recvBuf[recBufPos:])
			recBufPos += BufLen

			if _this.recvErro(BufLen <= 0) {
				return
			}

			if recBufPos != SOCKET_REV_BUF_SIZE {
				continue
			}
			recBufPos = 0

			_this.m_receiver.rev(recvBuf, Fun)
		}
	}()
}
func (_this *socketConn) recvErro(bErro bool) bool {
	if !bErro {
		return false
	}
	_this.m_bConnect = false
	_this.OnRevFinish(0, nil, 0)
	_this.m_Conn.Close()
	return true
}
func (_this *socketConn) Send(cmd uint32, msg []byte, msgSize int) bool {
	if !_this.m_bConnect {
		return false
	}
	return _this.m_sender.SendMsg(_this.m_Conn, cmd, msg, msgSize)
}

////////

///////ServerSocket
type ServerSocket struct {
	m_sockSrv        net.Listener
	m_pISocketRecver *IServerRecver

	m_Connect *list.List
	m_lock    *sync.Mutex
}

func (_this *ServerSocket) Start(port string, pISocketRecver *IServerRecver) bool {
	_this.m_lock = new(sync.Mutex)

	_this.m_Connect = list.New()

	_this.m_pISocketRecver = pISocketRecver

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

			_this.m_lock.Lock()
			_this.removeInvalid()
			connect := new(socketConn)
			_this.m_Connect.PushBack(connect)
			connect.init(conn, _this.m_pISocketRecver, nil)
			_this.m_lock.Unlock()
		}
	}()

	return true
}
func (_this *ServerSocket) SendToAllConn(cmd uint32, msg []byte, msgSize int) bool {
	return _this.sendToConn(cmd, msg, msgSize, ConnMark_Def)
}
func (_this *ServerSocket) SendToMarkConn(mark byte, cmd uint32, msg []byte, msgSize int) bool {
	return _this.sendToConn(cmd, msg, msgSize, mark)
}
func (_this *ServerSocket) removeInvalid() {
	temp := list.New()
	for i := _this.m_Connect.Front(); i != nil; i = i.Next() {
		var conn *socketConn = i.Value.(*socketConn)
		if conn.m_bConnect {
			temp.PushBack(i)
		}
	}
	_this.m_Connect = temp
}
func (_this *ServerSocket) sendToConn(cmd uint32, msg []byte, msgSize int, mark byte) bool {
	sendSuc := false
	_this.m_lock.Lock()

	for i := _this.m_Connect.Front(); i != nil; i = i.Next() {
		var conn *socketConn = i.Value.(*socketConn)
		if !conn.m_bConnect {
			continue
		}

		if mark != ConnMark_Def {
			if mark != conn.m_ConnMark {
				continue
			}
		}

		sendSuc = conn.Send(cmd, msg, msgSize)
	}

	_this.m_lock.Unlock()
	return sendSuc
}

//////

///////ClientSocket
type ClientSocket struct {
	m_Connect *socketConn
}

func (_this *ClientSocket) init() {
	_this.m_Connect = nil
}

//Start startClinet
func (_this *ClientSocket) Start(mark byte, Ip string, port string, recver *IClientRecver) bool {
	conn, err := net.DialTimeout("tcp", Ip+":"+port, 2*time.Second)
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

	_this.m_Connect = new(socketConn)
	_this.m_Connect.init(conn, nil, recver)
	return true
}

//Send ClinetSendMsg
func (_this *ClientSocket) Send(cmd uint32, msg []byte, msgSize int) bool {
	if nil == _this.m_Connect {
		return false
	}
	return _this.m_Connect.Send(cmd, msg, msgSize)
}

////////

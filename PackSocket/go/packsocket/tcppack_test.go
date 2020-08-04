package packsocket

import (
	"fmt"
	"testing"
)

type Client struct {
	m_cl ClientSocket
}

func (_this *Client) OnRec(cmd uint32, recvBuf []byte, BufLen int) {
	str := string(recvBuf[:BufLen])
	fmt.Println(str)
}
func (_this *Client) run() {
	_this.m_cl.init()
	var rev IClientRecver = _this
	_this.m_cl.Start(0, "127.0.0.1", "1234", &rev)

	bys := []byte("我是客户端")
	_this.m_cl.Send(0, bys, len(bys))

	for {
	}
}

func TestClient(t *testing.T) {
	obj := Client{}
	obj.run()
}

type Server struct {
	m_ser ServerSocket
}

func (_this *Server) OnRec(mark byte, cmd uint32, recvBuf []byte, BufLen int) {
	str := string(recvBuf[:BufLen])
	fmt.Println(str)
}
func (_this *Server) run() {
	var rev IServerRecver = _this
	_this.m_ser.Start("1234", &rev)

	for {
	}
}

func TestServer(t *testing.T) {
	obj := Server{}
	obj.run()
}

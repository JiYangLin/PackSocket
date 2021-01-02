package packsocket

import (
	"fmt"
	"testing"
)

type Client struct {
	mCl ClientSocket
}

func (_this *Client) onRec(data *RevData) {
	if nil ==  data{
		fmt.Println("disconnected ...");
		return;
	}
	str := string(data.recvBytes[:data.msglen])
	fmt.Println(str)
}
func (_this *Client) run() {
	_this.mCl.init()
	var rev IRev = _this
	_this.mCl.Start(0, "127.0.0.1", "1234", rev)
	bys := []byte("我是客户端")
	_this.mCl.Send(1, bys, len(bys))
	for {
	}
}

func TestClient(t *testing.T) {
	obj := Client{}
	obj.run()
}

type Server struct {
	mSer ServerSocket
}

func (_this *Server) onRec(data *RevData) {
	if nil ==  data{
		fmt.Println("disconnected ...");
		return;
	}

	str := string(data.recvBytes[:data.msglen])
	fmt.Println(str)
}
func (_this *Server) run() {
	var rev IRev = _this
	_this.mSer.Start("1234", rev)

	for {
	}
}

func TestServer(t *testing.T) {
	obj := Server{}
	obj.run()
}

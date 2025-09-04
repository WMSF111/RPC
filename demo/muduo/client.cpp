#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <iostream>
#include <string>

class DictClient {
    public:
        DictClient(const std::string &sip, int sport):
            _baseloop(_loopthread.startLoop()),
            _downlatch(1),//初始化计数器为1，因为为0时才会唤醒
            _client(_baseloop, muduo::net::InetAddress(sip, sport), "DictClient")
        {
            //设置连接事件（连接建立/管理）的回调
            _client.setConnectionCallback(std::bind(&DictClient::onConnection, this, std::placeholders::_1));
            //设置连接消息的回调
            _client.setMessageCallback(std::bind(&DictClient::onMessage, this, 
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            
            //连接服务器，与服务端不同的地方
            _client.connect(); 
            _downlatch.wait(); // 等待_downlatch为0
        }

        bool send(const std::string &msg) { //client无法发送信息，需要建立_conn
            if (_conn->connected() == false) {
                std::cout << "连接已经断开，发送数据失败！\n";
                return false;
            }
            _conn->send(msg);
            return true;
        }
    
    private:
        void onConnection(const muduo::net::TcpConnectionPtr &conn) {
            if (conn->connected()) {
                std::cout << "连接建立！\n";
                _downlatch.countDown();//计数--，为0时唤醒阻塞
                _conn = conn;
            }else {
                std::cout << "连接断开！\n";
                _conn.reset();
            }
        }
        // 翻译结果打印出来
        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp){
            std::string res = buf->retrieveAllAsString();
            std::cout << res << std::endl;
        }
    private:
        muduo::net::TcpConnectionPtr _conn; //确保链接建立成功
        muduo::CountDownLatch _downlatch; // 解决阻塞问题
        muduo::net::EventLoopThread _loopthread; // 解决死循环问题，无法发送数据 
        muduo::net::EventLoop *_baseloop; //事件循环（死循环）,通过指针可以灵活地引用不同类型的EventLoop对象
        muduo::net::TcpClient _client;
};

int main()
{
    DictClient client("127.0.0.1", 9090);
    while(1) {
        std::string msg;
        std::cin >> msg;
        client.send(msg);
    }
    return 0;
}
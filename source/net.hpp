#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/net/TcpServer.h>
#include "detail.hpp"
#include "fieIds.hpp"
#include "abstract.hpp"
#include "message.hpp"
#include <mutex>
#include <unordered_map>



namespace bitrpc{
    class MuduoBuffer : public BaseBuffer{ //缓存类
        public:
            using ptr = std::shared_ptr<MuduoBuffer>; 
            MuduoBuffer(muduo::net::Buffer *buf):_buf(buf) {}
            virtual size_t readableSize() override{ // 判断缓冲区可读数量
                return _buf->readableBytes();
            }; 
            virtual int32_t peekInt32() override{ // 从缓冲区取出4字节
                //muduo库是一个网络库，从缓冲区取出一个4字节整形，会进行网络字节序的转换
                return _buf->peekInt32();
            }; 
            virtual void retrieveInt32() override{ //从缓冲区删除4字节
                return _buf->retrieveInt32();
            };
            virtual int32_t readInt32() override{// 从缓冲区取出并删除4字节
                return _buf->readInt32();
            }; 
            virtual std::string retrieveAsString(size_t len) override{// 从缓冲区取出指定长度数据
                return _buf->retrieveAsString(len);
            }; 
        private:
            muduo::net::Buffer *_buf; //用指针：需要多个MuduoBuffer对象共享同一个Buffer对象
    };
    class BufferFactory{
        public:
        template<typename ...Args> // 模板参数包
        static BaseBuffer::ptr create(Args&& ...args) { // ptr 是 BaseBuffer 类的一个别名
                //Args&& ...args：这部分是模板参数包的参数
                return std::make_shared<MuduoBuffer>(std::forward<Args>(args)...);
                // std::forward是一种实现完美转发的技术，确保传递给MuduoBuffer构造函数的参数args保持其原始类型。
                // 必须显式指定模板参数类型<>
        }
    };

    class LVProtocol : public BaseProtocol { //通信协议类
        // |--Len--|--VALUE--|
        // |--Len--|--mtype--|--idlen--|--id--|--body--|
        public:
            using ptr = std::shared_ptr<LVProtocol>;
            virtual bool canProcessed(const BaseBuffer::ptr &buf){
                // 当缓冲区没有内存时，不能peekInt32
                if(buf->readableSize() < lenFieldsLength)    return false;  
                int32_t total_len = buf->peekInt32(); // 缓冲区读取4字节
                // 可使用内存小于需要的内存 len + len的值
                if(buf->readableSize() < (total_len + lenFieldsLength)) return false;
                return true;
            }; //判断是否能处理

            /*将缓冲区数据赋给msg*/
            virtual bool onMessage(const BaseBuffer::ptr &buf, BaseMessage::ptr &msg){
                //当调用onMessage的时候，默认认为缓冲区中的数据足够一条完整的消息
                int32_t total_len = buf->peekInt32(); // 读综合长度
                MType mtype =  (MType)buf->readInt32(); // 读取数据类型
                int32_t idlen = buf->readInt32(); // 读取id长度
                int32_t body_len = total_len - idlen - idlenFieldsLength - mtypeFieldsLength; 
                std::string id = buf->retrieveAsString(idlen);
                std::string body = buf->retrieveAsString(body_len);
                DLOG("LVProtocol消息body:%s", body.c_str());
                msg = MessageFactory::create(mtype);
                if (msg.get() == nullptr) {
                    ELOG("消息类型错误，构造消息对象失败！");
                    return false;
                }
                bool ret = msg->unserialize(body);
                if (ret == false) {
                    ELOG("消息正文反序列化失败！");
                    return false;
                }
                msg->setID(id);
                msg->setMtype(mtype);
                return true;
            }; //给buf发送msg信息

            /*将msg转为string*/
            virtual std::string serialize(const BaseMessage::ptr &msg) override{
                // |--Len--|--mtype--|--idlen--|--id--|--body--|
                std::string body = msg->serialize();
                DLOG("LV协议serialize消息body:%s", body.c_str());
                std::string id = msg->rid();
                // htonl:将主机字节顺序（通常是小端字节序）转换为网络字节顺序（大端字节序）
                auto mtype = htonl((int32_t)msg->mtype());
                int32_t idlen = htonl(id.size());
                int32_t h_total_len = mtypeFieldsLength + idlenFieldsLength + id.size() + body.size();
                int32_t n_total_len = htonl(h_total_len); // 转化后变得很大
                std::string result;
                // 计算和处理数据时，仍然处于主机字节顺序，将数据写入网络传输时，数据需要转化为网络字节顺序。
                result.reserve(h_total_len); // 开辟total_len空间，用的是转换前长度
                result.append((char*)&n_total_len, lenFieldsLength); // 这里用的是网络字节序
                result.append((char*)&mtype, mtypeFieldsLength);
                result.append((char*)&idlen, idlenFieldsLength);
                // 字符串本身是按字符的字节序列存储的，而字符（例如 ASCII 字符）在不同字节序的机器上通常是一样的
                result.append(id);
                result.append(body);
                for (char c : result) {
                    printf("%02x ", (unsigned char)c);
                }
                DLOG("LV协议serialize消息result:%s", result.c_str());
                fflush(stdout);  // 强制刷新输出缓冲区
                return result;
            }; //对消息序列化

        private:
            const size_t lenFieldsLength = 4;
            const size_t mtypeFieldsLength = 4;
            const size_t idlenFieldsLength = 4;
    };
     class ProtocolFactory {
        public:
            template<typename ...Args>
            static BaseProtocol::ptr create(Args&& ...args) {
                return std::make_shared<LVProtocol>(std::forward<Args>(args)...);
            }
    };

    /*建立Muduo的链接，其通讯协议为_protocol*/
    class MuduoConnection : public BaseConnection { // Connect方法是使用Muduo实现的
        public:
            using ptr = std::shared_ptr<MuduoConnection>;
            MuduoConnection(const muduo::net::TcpConnectionPtr &conn, 
                const BaseProtocol::ptr &protocol) : 
                _protocol(protocol), _conn(conn) {}

            virtual void send(const BaseMessage::ptr &msg) override {
                std::string body = _protocol->serialize(msg); //按照协议转化
                DLOG("MuduoConnectionsend消息body:%s", body.c_str());
                _conn->send(body);
            }

            virtual void shutdown() override {
                _conn->shutdown();
            }

            virtual bool connected() override {
                return _conn->connected();
            }
        private: //链接主要是基于TcpConnectionPtr和BaseProtocol
            // 一般不需要为每一个server构造独立protocol，造成浪费
            BaseProtocol::ptr _protocol;
            muduo::net::TcpConnectionPtr _conn;
    };
    class ConnectionFactory {
        public:
            template<typename ...Args>
            static BaseConnection::ptr create(Args&& ...args) {
                return std::make_shared<MuduoConnection>(std::forward<Args>(args)...);
            }
    };

    class MuduoServer : public BaseServer{ // 通过Muduo建立Server
        public:
            using ptr = std::shared_ptr<MuduoServer>;
            MuduoServer(int port): 
            _server(&_evenloop, muduo::net::InetAddress(port),
                "MuduoServer", muduo::net::TcpServer::kReusePort),
            _protocol(ProtocolFactory::create()){} // protocol需要提前创建，否则会报错
        void start(){
           //设置连接事件（连接建立/管理）的回调
            _server.setConnectionCallback(std::bind(&MuduoServer::onConnection, this, std::placeholders::_1));
            //设置连接消息的回调
            _server.setMessageCallback(std::bind(&MuduoServer::onMessage, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _server.start(); //开始监听
            _evenloop.loop(); //事件死循环
        }
    private: 
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if(conn->connected()){ // 将链接存到_conns中并回调_cb_connection
                std::cout << "连接建立！\n";
                auto muduo_conn = ConnectionFactory::create(conn, _protocol); //建立链接关系
                {
                    std::unique_lock<std::mutex> lock(_mutex); // 建立动态锁，出作用域就释放
                    _conns.insert(std::make_pair(conn, muduo_conn)); //将建立的链接插入_conns
                }
                // 如果_cb_connection不为空，则调用它，并将 muduo_conn作为参数传递给它
                if(_cb_connection) _cb_connection(muduo_conn);
            }else{ // 将断开的链接从_conns中删除并回调_cb_close
                std::cout <<"连接断开！\n";
                BaseConnection::ptr muduo_conn;
                {
                    std::unique_lock<std::mutex> lock(_mutex); // 建立动态锁，出作用域就释放
                    auto it = _conns.find(conn);
                    if(it == _conns.end()) return;
                    muduo_conn = it->second;
                    _conns.erase(conn);
                }if(_cb_close) _cb_close(muduo_conn);
                }
        }

        void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
        {
            // 在处理msg回调前需要确定msg准确性，处理buf中的消息化为lv协议
            DLOG("连接有数据到来，开始处理！");
            auto base_buf = BufferFactory::create(buf); //buf中有很多数据，先封装
            while(1) {
                // 1. 检查缓冲区空间是否足够
                if (_protocol->canProcessed(base_buf) == false) { //看能否处理base_buf
                    //缓冲区数据不足
                    if (base_buf->readableSize() > maxDataSize) { // 缓冲区数据过多
                            conn->shutdown();
                            ELOG("缓冲区中数据过大！");
                            return ;
                        }
                        DLOG("数据量不足！");
                        break;
                }
                // 2. 将缓冲区信息存到msg中
                DLOG("缓冲区中数据可处理！");
                BaseMessage::ptr msg; // 要接收的信息
                bool ret = _protocol->onMessage(base_buf, msg); // 把basebuf信息发给msg
                if (ret == false) { // 数据有问题
                    // conn->shutdown();
                    ELOG("缓冲区中数据错误！");
                    return ;
                }
                // 3. 给conn的客户端发送msg
                DLOG("消息反序列化成功！")
                BaseConnection::ptr base_conn; //查找对应base_conn
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(conn);
                    if (it == _conns.end()) {
                        conn->shutdown();
                        return;
                    }
                    base_conn = it->second;
                }
                DLOG("调用回调函数进行消息处理！");
                if (_cb_message) _cb_message(base_conn, msg);
            }
        }
        private:
            const size_t maxDataSize = (1 << 16);
            muduo::net::TcpServer _server;
            muduo::net::EventLoop _evenloop;
            std::mutex _mutex;
            BaseProtocol::ptr _protocol; // ConnectionFactory需要_protocol和_conn
            // ConnectionFactory对BaseConnection进行创建，MuduoConnection中_conn是TcpConnectionPtr
            std::unordered_map<muduo::net::TcpConnectionPtr, BaseConnection::ptr> _conns;
    };
    class ServerFactory {
        public:
            template<typename ...Args>
            static BaseServer::ptr create(Args&& ...args) {
                return std::make_shared<MuduoServer>(std::forward<Args>(args)...);
            }
    };

    class MuduoClient : public BaseClient {
        public:
            using ptr = std::shared_ptr<MuduoClient>;
            MuduoClient(const std::string &sip, int sport):
                _protocol(ProtocolFactory::create()),
                _baseloop(_loopthread.startLoop()),
                _downlatch(1), //初始化计数器为1，因为为0时才会唤醒
                _client(_baseloop, muduo::net::InetAddress(sip, sport), "MuduoClient"){}
            virtual void connect() override {
                DLOG("设置回调函数，连接服务器");
                //设置连接事件（连接建立/管理）的回调
                _client.setConnectionCallback(std::bind(&MuduoClient::onConnection, this, std::placeholders::_1));
                //设置连接消息的回调
                _client.setMessageCallback(std::bind(&MuduoClient::onMessage, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                
                //连接服务器
                _client.connect();
                _downlatch.wait();
                DLOG("连接服务器成功！");
            }
            virtual void shutdown() override {
                return _client.disconnect();
            }
            virtual bool send(const BaseMessage::ptr &msg) override { 
                if (connected() == false) {
                    ELOG("连接已断开！");
                    return false;
                }
                _conn->send(msg);
                return true;
            }
            virtual BaseConnection::ptr connection() override { // 返回链接的对象
                return _conn;
            }
            virtual bool connected() { // 判断链接状态
                return (_conn && _conn->connected()); // 客户端只有一个链接
            }
        private:
            /*建立_conn连接， conn为要建立的链接指针*/
            void onConnection(const muduo::net::TcpConnectionPtr &conn) { 
                if (conn->connected()) {
                    std::cout << "连接建立！\n";
                    _downlatch.countDown();//计数--，为0时唤醒阻塞
                    _conn = ConnectionFactory::create(conn, _protocol);
                }else {
                    std::cout << "连接断开！\n";
                    _conn.reset();
                }
            }
            /*给链接的服务端发送消息*/
            void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp){
                DLOG("连接有数据到来，开始处理！");
                auto base_buf = BufferFactory::create(buf);
                while(1) {
                    /*1.判断缓冲区空间是否充足*/
                    if (_protocol->canProcessed(base_buf) == false) {
                        //数据不足
                        if (base_buf->readableSize() > maxDataSize) {
                            conn->shutdown();
                            ELOG("缓冲区中数据过大！");
                            return ;
                        }
                        DLOG("数据量不足！");
                        break;
                    }
                    /*2. 将缓冲区数据处理成msg*/
                    DLOG("缓冲区中数据可处理！");
                    BaseMessage::ptr msg;
                    bool ret = _protocol->onMessage(base_buf, msg);
                    if (ret == false) {
                        conn->shutdown();
                        ELOG("缓冲区中数据错误！");
                        return ;
                    }
                    /*3.给链接的_conn发送消息*/
                    DLOG("缓冲区中数据解析完毕，调用回调函数进行处理！");
                    if (_cb_message) _cb_message(_conn, msg);
                }
            }
        private:
            const size_t maxDataSize = (1 << 16);
            BaseProtocol::ptr _protocol; // 通讯协议
            BaseConnection::ptr _conn; //确保链接建立成功
            muduo::CountDownLatch _downlatch; // 解决阻塞问题
            muduo::net::EventLoopThread _loopthread; // 解决死循环问题，无法发送数据 
            muduo::net::EventLoop *_baseloop; //事件循环（死循环）,通过指针可以灵活地引用不同类型的EventLoop对象
            muduo::net::TcpClient _client;
    };
    
    class ClientFactory {
        public:
            template<typename ...Args>
            static BaseClient::ptr create(Args&& ...args) {
                return std::make_shared<MuduoClient>(std::forward<Args>(args)...);
            }
    };
}
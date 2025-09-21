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
            using ptr = std::shared_ptr<BaseBuffer>; 
            MuduoBuffer(muduo::net::Buffer *buf):_buf(buf) {}
            virtual size_t readableSize() override{
                return _buf->readableBytes();
            }; // 判断缓冲区可读数量
            virtual int32_t peekInt32() override{
                //muduo库是一个网络库，从缓冲区取出一个4字节整形，会进行网络字节序的转换
                return _buf->peekInt32();
            }; // 从缓冲区取出4字节
            virtual void retrieveInt32() override{
                return _buf->retrieveInt32();
            }; //从缓冲区删除4字节
            virtual int32_t readInt32() override{
                return _buf->readInt32();
            }; // 从缓冲区取出并删除4字节
            virtual std::string retrieveAsString(size_t len) override{
                return _buf->retrieveAsString(len);
            }; // 从缓冲区取出指定长度数据
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
        }
    };

    class LVProtocol : BaseProtocol { //通信协议类
        // |--Len--|--VALUE--|
        // |--Len--|--mtype--|--idlen--|--id--|--body--|
        public:
            using ptr = std::shared_ptr<BaseProtocol>;
            virtual bool canProcessed(const BaseBuffer::ptr &buf){
                int32_t total_len = buf->peekInt32();
                if(buf->readableSize() < (total_len + lenFieldsLength)) return false;
                return true;
            }; //判断是否能处理

            virtual bool onMessage(const BaseBuffer::ptr &buf, BaseMessage::ptr &msg){
                //当调用onMessage的时候，默认认为缓冲区中的数据足够一条完整的消息
                int32_t total_len = buf->peekInt32(); // 读综合长度
                MType mtype =  (MType)buf->readInt32(); // 读取数据类型
                int32_t idlen = buf->readInt32(); // 读取id长度
                int32_t body_len = total_len - idlen - idlenFieldsLength - mtypeFieldsLength; 
                std::string id = buf->retrieveAsString(idlen);
                std::string body = buf->retrieveAsString(body_len);
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
            virtual std::string serialize(const BaseMessage::ptr &msg){
            // |--Len--|--mtype--|--idlen--|--id--|--body--|
            std::string body = msg->serialize();
            std::string id = msg->rid();
            // htonl:将主机字节顺序（通常是小端字节序）转换为网络字节顺序（大端字节序）
            auto mtype = htonl((int32_t)msg->mtype());
            int32_t idlen = htonl(id.size());
            int32_t h_total_len = mtypeFieldsLength + idlenFieldsLength + id.size() + body.size();
            int32_t n_total_len = htonl(h_total_len);
            std::string result;
            result.reserve(h_total_len); // 开辟total_len空间
            result.append((char*)&n_total_len, lenFieldsLength);
            result.append((char*)&mtype, mtypeFieldsLength);
            result.append((char*)&idlen, idlenFieldsLength);
            // 字符串本身是按字符的字节序列存储的，而字符（例如 ASCII 字符）在不同字节序的机器上通常是一样的
            result.append(id);
            result.append(body);
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

    class MuduoConnection : public BaseConnection {
        public:
            using ptr = std::shared_ptr<MuduoConnection>;
            MuduoConnection(const muduo::net::TcpConnectionPtr &conn, 
                const BaseProtocol::ptr &protocol) : 
                _protocol(protocol), _conn(conn) {}

            virtual void send(const BaseMessage::ptr &msg) override {
                std::string body = _protocol->serialize(msg);
                _conn->send(body);
            }

            virtual void shutdown() override {
                _conn->shutdown();
            }

            virtual bool connected() override {
                _conn->connected();
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
}
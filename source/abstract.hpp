#pragma once
#include<memory>
#include<functional>
#include "fieIds.hpp"

namespace bitrpc{
    class BaseMessage{ // 消息类
        public:
            // 用于管理动态分配的内存资源（堆内存），并且通过引用计数机制来自动控制内存的释放。
            using ptr = std::shared_ptr<BaseMessage>;  // 用ptr代表共享指针，方便管理
            virtual ~BaseMessage(){} //析构,声明需要分号，定义不需要
            virtual void setID(const std::string &id){ _rid = id;} // 大对象一般使用const+&
            virtual std::string rid(){return _rid;}
            virtual void setMtype(MType mtype){_mtype = mtype;} // 小对象且可更改，会有个mtype副本
            virtual MType mtype(){return _mtype;}
            // =0 意味着这些函数没有提供实现，而是要求派生类必须提供它们的具体实现。
            virtual std::string serialize() = 0; 
            virtual bool unserialize(const std::string &msg) = 0; 
            virtual bool check() = 0; 
        private:
            MType _mtype;
            std::string _rid;
    };

    class BaseBuffer{ //缓存类
        public:
            using ptr = std::shared_ptr<BaseBuffer>; 
            virtual size_t readableSize() = 0; // 判断缓冲区可读数量
            virtual int32_t peekInt32() = 0; // 从缓冲区取出4字节
            virtual void retrieveInt32() = 0; //从缓冲区删除4字节
            virtual int32_t readInt32() = 0; // 从缓冲区取出并删除4字节
            virtual std::string retrieveAsString(size_t len) = 0; // 从缓冲区取出指定长度数据
    };

    class BaseProtocol { //通信协议类
        public:
            using ptr = std::shared_ptr<BaseProtocol>;
            virtual bool canProcessed(const BaseBuffer::ptr &buf) = 0; //判断是否能处理
            virtual bool onMessage(const BaseBuffer::ptr &buf,
            BaseMessage::ptr &msg) = 0; //给buf发送msg信息
            virtual std::string serialize(const BaseMessage::ptr &msg) = 0; //对消息序列化
    };

    class BaseConnection { //通讯链接类
        public:
            using ptr = std::shared_ptr<BaseConnection>;
            virtual void send(const BaseMessage::ptr &msg) = 0; // 发送消息
            virtual void shutdown() = 0; // 通讯下线
            virtual bool connected() = 0; // 判断通讯是否连接
    };

    // 定义为一个类型别名ConnectionCallback，表示一个接受 BaseConnection::ptr类型参数并返回void的函数
    using ConnectionCallback = std::function<void(const BaseConnection::ptr&)>;
    using CloseCallback = std::function<void(const BaseConnection::ptr&)>;
    using MessageCallback = std::function<void(const BaseConnection::ptr&, BaseMessage::ptr&)>;
    class BaseServer { //服务端抽象类
        public:
            using ptr = std::shared_ptr<BaseServer>;
            // 接受一个ConnectionCallback类型的回调函数cb作为参数
            virtual void setConnectionCallback(const ConnectionCallback& cb) { // 设置链接回调函数
                _cb_connection = cb; //将传入的回调函数cb赋值给成员变量_cb_connection，以便存储连接回调函数。
            }
            virtual void setCloseCallback(const CloseCallback& cb) { // 设置关闭回调函数
                _cb_close = cb;
            }
            virtual void setMessageCallback(const MessageCallback& cb) { // 设置消息发送回调函数
                _cb_message = cb;
            }
            virtual void start() = 0; 
        protected:
            ConnectionCallback _cb_connection;
            CloseCallback _cb_close;
            MessageCallback _cb_message;
    };

    class BaseClient { //客户端抽象类
        public:
            using ptr = std::shared_ptr<BaseClient>;
            virtual void setConnectionCallback(const ConnectionCallback& cb) {
                _cb_connection = cb;
            }
            virtual void setCloseCallback(const CloseCallback& cb) {
                _cb_close = cb;
            }
            virtual void setMessageCallback(const MessageCallback& cb) {
                _cb_message = cb;
            }
            virtual void connect() = 0; // 链接服务器
            virtual void shutdown() = 0; // 关闭服务器
            virtual bool send(const BaseMessage::ptr&) = 0; // 发送消息
            virtual BaseConnection::ptr connection() = 0; // 获取连接对象
            virtual bool connected() = 0; //判断链接是否正常
        protected:
            ConnectionCallback _cb_connection;
            CloseCallback _cb_close;
            MessageCallback _cb_message;
    };
}
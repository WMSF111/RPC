#pragma once
#include "net.hpp"
#include "message.hpp"

namespace bitrpc {
    class Dispatcher{
        public:
            using ptr = std::shared_ptr<Dispatcher>;
            // cb是传入的一个std::function 类型，其本身通常会存储回调函数的副本，函数类型参数应当被标记为 const
            void registerHandler(MType mytype, const MessageCallback &cb){
                std::unique_lock<std::mutex> lock(_mutex);
                _handler.insert(std::make_pair(mytype, cb));
            }

            void onMessage(const BaseConnection::ptr& conn, BaseMessage::ptr& msg){ //回调函数
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _handler.find(msg->mtype());
                if(it != _handler.end()) {
                    return it->second(conn, msg); //返回回调函数
                }
                ELOG("收到未知类型消息:%d ！" , (int)msg->mtype());
                conn->shutdown();
            }
        
        private:
            std::mutex _mutex;
            std::unordered_map<MType, MessageCallback> _handler;
    };
}
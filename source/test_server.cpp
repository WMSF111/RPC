#include "dispatcher.hpp"

// 收到请求
void onRequestRcp(const bitrpc::BaseConnection::ptr& conn, bitrpc::BaseMessage::ptr& msg){
    std::string body = msg->serialize(); // 解析成string数据
    std::cout << body<< std::endl;
    auto rpc_req = bitrpc::MessageFactory::create<bitrpc::RpcResponse>(); // 建立应答
    rpc_req->setID("11111");
    rpc_req->setMtype(bitrpc::MType::RSP_RPC);
    rpc_req->setRCode(bitrpc::RCode::RCODE_OK);
    rpc_req->setResult(33);
    conn->send(rpc_req); // 发送应答
}

void onRequestTopic(const bitrpc::BaseConnection::ptr& conn, bitrpc::BaseMessage::ptr& msg){
    std::string body = msg->serialize(); // 解析成string数据
    std::cout << body<< std::endl;
    auto rpc_req = bitrpc::MessageFactory::create<bitrpc::TopicResponse>(); // 建立应答
    rpc_req->setID("22222");
    rpc_req->setMtype(bitrpc::MType::RSP_TOPIC);
    rpc_req->setRCode(bitrpc::RCode::RCODE_OK);
    conn->send(rpc_req); // 发送应答
}

int main()
{
    auto dispatcher = std::make_shared<bitrpc::Dispatcher>();
    dispatcher->registerHandler(bitrpc::MType::REQ_RPC, onRequestRcp); // 收到了请求的回调
    dispatcher->registerHandler(bitrpc::MType::REQ_TOPIC, onRequestTopic);
    auto message_cb = std::bind(&bitrpc::Dispatcher::onMessage, dispatcher.get(),
            std::placeholders::_1, std::placeholders::_2);
    auto server = bitrpc::ServerFactory::create(9090);
    server->setMessageCallback(message_cb); // 父类
    server->start(); // 子类，会确保消息准确
    return 0;
}


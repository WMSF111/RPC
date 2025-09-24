#include "net.hpp"
#include "message.hpp"

void onMessage(const bitrpc::BaseConnection::ptr& conn, bitrpc::BaseMessage::ptr& msg){
    std::string body = msg->serialize(); // 解析成string数据
    std::cout << body<< std::endl;
    auto rpc_req = bitrpc::MessageFactory::create<bitrpc::RpcResponse>(); // 建立应答
    rpc_req->setID("11111");
    rpc_req->setMtype(bitrpc::MType::RSP_RPC);
    rpc_req->setRCode(bitrpc::RCode::RCODE_OK);
    rpc_req->setResult(33);
    conn->send(rpc_req); // 发送应答
}

int main()
{
    auto server = bitrpc::ServerFactory::create(9090);
    server->setMessageCallback(onMessage); // 父类
    server->start(); // 子类，会确保消息准确
    return 0;
}


#include "net.hpp"
#include<thread>
#include "message.hpp"


void onMessage(const bitrpc::BaseConnection::ptr& conn, bitrpc::BaseMessage::ptr& msg){
    std::string body = msg->serialize(); // 解析成string数据
    std::cout << body<< std::endl;
    
}

int main()
{
    auto client = bitrpc::ClientFactory::create("127.0.0.1", 9090);
    client->setMessageCallback(onMessage); // 父类
    client->connect(); // 链接服务器
    auto rpc_req = bitrpc::MessageFactory::create<bitrpc::RpcRequest>(); // 建立需求
    rpc_req->setID("11111");
    rpc_req->setMtype(bitrpc::MType::REQ_RPC);
    rpc_req->setMethod("ADD");
    Json::Value params;
    params["num1"] = 11;
    params["num2"] = 22;
    rpc_req->setParams(params);
    client->send(rpc_req); // 发送应答
    std::this_thread::sleep_for(std::chrono::seconds(10));
    client->shutdown();
    return 0;
}


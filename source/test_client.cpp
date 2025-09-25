#include<thread>
#include "dispatcher.hpp"
// 客户端收到响应
void onResponceRpc(const bitrpc::BaseConnection::ptr& conn, bitrpc::BaseMessage::ptr& msg){
    std::cout << "收到了RPC响应" << std::endl;
    std::string body = msg->serialize(); // 解析成string数据
    std::cout << body<< std::endl;
}

void onResponceTopic(const bitrpc::BaseConnection::ptr& conn, bitrpc::BaseMessage::ptr& msg){
    std::cout << "收到了TOPIC响应" << std::endl;
    std::string body = msg->serialize(); // 解析成string数据
    std::cout << body<< std::endl;
}

int main()
{
    auto dispatcher = std::make_shared<bitrpc::Dispatcher>();
    dispatcher->registerHandler(bitrpc::MType::RSP_RPC, onResponceRpc); //收到了响应的回调
    dispatcher->registerHandler(bitrpc::MType::RSP_TOPIC, onResponceTopic);

    auto client = bitrpc::ClientFactory::create("127.0.0.1", 9090);
    // message_cb 是一个回调函数对象.
    // 它将Dispatcher::onMessage函数与dispatcher对象绑定在一起，并为后续的消息处理提供了一个统一的接口。
    auto message_cb = std::bind(&bitrpc::Dispatcher::onMessage, dispatcher.get(),
            std::placeholders::_1, std::placeholders::_2);
    client->setMessageCallback(message_cb); // 父类
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

    auto topic_req = bitrpc::MessageFactory::create<bitrpc::TopicRequest>(); // 建立需求
    topic_req->setID("22222");
    topic_req->setMtype(bitrpc::MType::REQ_TOPIC);
    topic_req->setOptype(bitrpc::TopicOptype::TOPIC_PUBLISH);
    topic_req->setTopicKey("topickey");
    topic_req->setTopicMsg("Topicmsg");
    
    client->send(topic_req); // 发送应答

    std::this_thread::sleep_for(std::chrono::seconds(10));
    client->shutdown();
    return 0;
}


#include "message.hpp"

int main()
{
    //RPCRequest测试
    // bitrpc::RpcRequest::ptr rqt = bitrpc::MessageFactory::create<bitrpc::RpcRequest>();
    // Json::Value prama;
    // prama["num1"] = 11;
    // prama["num2"] = 22;
    // rqt->setMethod("ADD");
    // rqt->setParams(prama);
    // std::string str = rqt->serialize();
    // std::cout << str << std::endl;

    // bitrpc::BaseMessage::ptr msg = bitrpc::MessageFactory::create(bitrpc::MType::REQ_RPC);
    // bool ret = msg->unserialize(str);
    // if(ret == false) return -1;
    // bitrpc::RpcRequest::ptr rqt2 = std::dynamic_pointer_cast<bitrpc::RpcRequest>(msg);
    // std::cout << rqt2->params()["num1"].asInt() << std::endl;
    // std::cout << rqt2->params()["num2"].asInt() << std::endl;
    // std::cout << rqt2->method() << std::endl;
    
    //消息请求
    // bitrpc::TopicRequest::ptr rqt = bitrpc::MessageFactory::create<bitrpc::TopicRequest>();
    // rqt->setTopicKey("new");
    // rqt->setOptype(bitrpc::TopicOptype::TOPIC_PUBLISH);
    // rqt->setTopicMsg("hello world");
    // std::string str = rqt->serialize();
    // std::cout << str << std::endl;

    // bitrpc::BaseMessage::ptr msg = bitrpc::MessageFactory::create(bitrpc::MType::REQ_TOPIC);
    // bool ret = msg->unserialize(str);
    // if(ret == false) return -1;
    // bitrpc::TopicRequest::ptr rqt2 = std::dynamic_pointer_cast<bitrpc::TopicRequest>(msg);
    // rqt2->check();
    // std::cout << rqt2->topicKey() << std::endl;
    // std::cout << rqt2->topicMsg() << std::endl;
    return 0;
}


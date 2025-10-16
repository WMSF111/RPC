#include "../../common/dispatcher.hpp"
#include "../../server/rpc_router.hpp"

// 收到请求
void add(const Json::Value &req, Json::Value &rsp)
{
    int num1 = req["num1"].asInt();
    int num2 = req["num2"].asInt();
    rsp = num1 + num2;
}

int main()
{
//一、注册提供的服务
    //1.构造负责处理客户端请求的路由类(注册具体提供的服务)
    // 允许多个智能指针共享对对象的所有权，适用于多地方共享资源的情况；
    auto router = std::make_shared<bitrpc::server::RpcRouter>();
    // 希望一个对象只有一个拥有者时，使用 std::unique_ptr。
    //2.实例化工程对象
    std::unique_ptr<bitrpc::server::SDescribeFactory> desc_factory(new bitrpc::server::SDescribeFactory());
    desc_factory->setMethodName("Add");
    desc_factory->setParamsDesc("num1", bitrpc::server::VType::INTEGRAL);
    desc_factory->setParamsDesc("num2", bitrpc::server::VType::INTEGRAL);
    desc_factory->setReturnType(bitrpc::server::VType::INTEGRAL);
    desc_factory->setCallback(add);
    //3.生产的对象注册到router模块
    router->registerMethod(desc_factory->build());
//二、服务器针对RPC处理
    //1.确定服务器的rpc请求回调（根据提供的服务，router中的RpcRouter对收到的消息进行处理）
    auto cb = std::bind(&bitrpc::server::RpcRouter::onRequest, router.get(),
            std::placeholders::_1, std::placeholders::_2);

    //2.将rpc回调注册到dispatcher（2.dispatcher收到消息找到映射REQ_RPC后通过router处理）
    //建立消息类型与回调函数映射关系
    auto dispatcher = std::make_shared<bitrpc::Dispatcher>();
    dispatcher->registerHandler<bitrpc::RpcRequest>(bitrpc::MType::REQ_RPC, cb); // 收到了请求的回调
    //3.将dispatcher回调注册到server中（1.server收到消息通过dispatcher处理）
    auto message_cb = std::bind(&bitrpc::Dispatcher::onMessage, dispatcher.get(),
            std::placeholders::_1, std::placeholders::_2);
    auto server = bitrpc::ServerFactory::create(9090);
    server->setMessageCallback(message_cb); // 父类
    server->start(); // 子类，会确保消息准确
    return 0;
}


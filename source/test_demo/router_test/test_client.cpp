#include<thread>
#include "../../common/dispatcher.hpp"
#include "../../client/rpc_caller.hpp"
#include "../../client/requestor.hpp"

void callback(const Json::Value& result)
{
    ILOG("callback_result: %d", result.asInt());
}

int main()
{
//一、构造请求及映射
    // 1.初始化客户端请求对象(构造对象)
    auto requestor = std::make_shared<bitrpc::client::Requestor>(); 
    // 2.初始化客户端rpc请求对象(针对requestor)
    auto caller = std::make_shared<bitrpc::client::RpcCaller>(requestor);
    // 3.初始化分配请求对象映射的dispatcher对象
    auto dispatcher = std::make_shared<bitrpc::Dispatcher>();
    // 4.dispatcher绑定Requestor的onResponse获取的值传给Requestor::onResponse
    auto rsp_cb = std::bind(&bitrpc::client::Requestor::onResponse, requestor.get(),
        std::placeholders::_1, std::placeholders::_2);
        // 客户端请求的onResponse，msg类型是BaseMessage,registerHandler需要BaseMessage
    //RSP_RPC映射rsp_cb
    dispatcher->registerHandler<bitrpc::BaseMessage>(bitrpc::MType::RSP_RPC, rsp_cb); //收到了响应的回调
//二、客户端构造发送的请求
    //1. 构造客户端对象
    auto client = bitrpc::ClientFactory::create("127.0.0.1", 9090);
        // message_cb 是一个回调函数对象.
        // 它将Dispatcher::onMessage函数与dispatcher对象绑定在一起，并为后续的消息处理提供了一个统一的接口。
    //2. client绑定Dispatcher的onMessage（dispatcher获取的值传给Dispatcher::onMessage）
    auto message_cb = std::bind(&bitrpc::Dispatcher::onMessage, dispatcher.get(),
            std::placeholders::_1, std::placeholders::_2);
    client->setMessageCallback(message_cb); // 父类
    client->connect(); // 链接服务器
    //3. 构建客户端rpc请求对象(针对requestor)
    auto conn = client->connection();
    Json::Value params, result;
    params["num1"] = 11;
    params["num2"] = 22;
    //4. caller发送请求
    bool ret = caller->call(conn,"Add",params,result);
    if(ret != false) std::cout<< "result: "<< result.asInt() << std::endl;

    bitrpc::client::RpcCaller::JsonAsyncResponse res_future;
    params["num1"] = 33;
    params["num2"] = 44;
    //4. caller发送请求
    ret = caller->call(conn,"Add",params,res_future);
    if(ret != false)
    {
        result = res_future.get();
        std::cout<< "result: "<< result.asInt() << std::endl;
    } 

    params["num1"] = 55;
    params["num2"] = 66;
    //4. caller发送请求
    ret = caller->call(conn,"Add", params, callback);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    client->shutdown();
    return 0;
}


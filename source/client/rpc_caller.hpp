#pragma once
/*针对RPC请求进行处理*/
#include "requestor.hpp"

namespace bitrpc{
    namespace client{
        class RpcCaller{
            public:
                using ptr = std::shared_ptr<RpcCaller>;
                // 设置异步call的响应类型
                using JsonAsyncResponse = std::future<Json::Value>; 
                // 设置回调函数的函数模板，输入参数是Json::Value
                using JsonResponseCallback = std::function<void(const Json::Value&)>; //注意要加上&
                
                RpcCaller(const Requestor::ptr &requestor) : _requestor(requestor){}

                //requestor中的send处理是针对BaseMessage进行处理的
                //用于在rpccaller中针对结果的处理是针对 RpcResponse里边的result进行的
                // 同步
                // BaseConnection是一个基类，可能会有多个派生类。通过使用智能指针，可以利用多态性，传递基类指针指向派生类的对象。
                bool call(const BaseConnection::ptr &conn, const std::string &method,
                        const Json::Value &params, Json::Value &result){
                    //1.组织请求
                    auto req_msg = MessageFactory::create<RpcRequest>(); //构造响应
                    req_msg->setID(UUID::uuid());
                    req_msg->setMethod(method);
                    req_msg->setMtype(MType::REQ_RPC);
                    req_msg->setParams(params);
                    BaseMessage::ptr rsp_msg; //构造响应信息
                    //2.服务器发送请求
                    //send是Base级别，需要传入Base类型
                    bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), rsp_msg);
                    if (ret == false) {
                        ELOG("同步Rpc请求失败！");
                        return false;
                    }
                    DLOG("收到响应，进行解析，获取结果!");
                    //3. 等待响应
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(rsp_msg); //响应信息转化为RPC类型
                    if (!rpc_rsp_msg) {
                        ELOG("rpc响应，向下类型转换失败！");
                        return false;
                    }
                    if (rpc_rsp_msg->rcode() != RCode::RCODE_OK) {
                        ELOG("rpc请求出错：%s", errReason(rpc_rsp_msg->rcode()));
                        return false;
                    }
                    result = rpc_rsp_msg->result(); // 设置result内容
                    DLOG("结果设置完毕！");
                    return true;
                }
                // 异步
                bool call(const BaseConnection::ptr &conn, const std::string &method,
                        const Json::Value &params, JsonAsyncResponse &result){
                    // 1.组织请求
                    // 设置回调函数（利用回调完成异步），回调函数中会传入一个promise对象，在回调函数中去堆promise设置数据
                    // 因为返回的时Message，用户需要Json
                    auto req_msg = MessageFactory::create<RpcRequest>();
                    req_msg->setID(UUID::uuid());
                    req_msg->setMtype(MType::REQ_RPC);
                    req_msg->setMethod(method);
                    req_msg->setParams(params);
                    // 不可以std::promise<Json::Value> json_promise;生命周期太短了
                    // 2. 构造响应请求
                    auto json_promise = std::make_shared<std::promise<Json::Value>>() ; //构造响应请求
                    result = json_promise->get_future(); //构造响应结果
                    // std::bind：是一个函数模板，用来创建一个新的可调用对象cb，这个新对象绑定了一个函数（成员函数、普通函数等）和部分参数。后续调用这个新对象时，缺失的参数会被传入。
                    // this：因为 Callback 是 RpcCaller 类的成员函数。 这样，当回调被调用时，它就知道是哪个对象来执行该成员函数。
                    // json_promise 必须是与 RpcCaller::Callback 的第一个参数类型一致。
                    // std::placeholders::_1：这是一个占位符，表示当调用新创建的函数对象时，调用者需要提供一个参数，这个参数将会传递给 Callback 函数的第二个参数。
                    // 3. 关联RequestCallback回调函数
                    Requestor::RequestCallback cb = std::bind(&RpcCaller::Callback, 
                        this, json_promise, std::placeholders::_1);
                    //4. 向服务器发送回调异步请求
                    bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), cb);
                    if (ret == false) {
                        ELOG("异步Rpc请求失败！");
                        return false;
                    }
                    return true;
                }
                //回调
                bool call(const BaseConnection::ptr &conn, const std::string &method,
                    const Json::Value &params, const JsonResponseCallback &cb) {
                    // 1. 构造响应请求
                    auto req_msg = MessageFactory::create<RpcRequest>();
                    req_msg->setID(UUID::uuid());
                    req_msg->setMtype(MType::REQ_RPC);
                    req_msg->setMethod(method);
                    req_msg->setParams(params);
                    // 2. 关联回调函数
                    Requestor::RequestCallback req_cb = std::bind(&RpcCaller::Callback1, 
                        this, cb, std::placeholders::_1);
                    // 3. 服务器发送回调请求
                    bool ret = _requestor->send(conn, std::dynamic_pointer_cast<BaseMessage>(req_msg), req_cb);
                    if (ret == false) {
                        ELOG("回调Rpc请求失败！");
                        return false;
                    }
                    return true;
                }
            private:
                // 普通回调
                void Callback1(const JsonResponseCallback &cb, const BaseMessage::ptr &msg)  {
                    //将msg转换成RpcResponse
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                    if (!rpc_rsp_msg) {
                        ELOG("rpc响应，向下类型转换失败！");
                        return ;
                    }
                    if (rpc_rsp_msg->rcode() != RCode::RCODE_OK) {
                        ELOG("rpc回调请求出错：%s", errReason(rpc_rsp_msg->rcode()));
                        return ;
                    }
                    cb(rpc_rsp_msg->result());
                }
                //异步回调
                void Callback(std::shared_ptr<std::promise<Json::Value>> result, const BaseMessage::ptr &msg)  {
                    auto rpc_rsp_msg = std::dynamic_pointer_cast<RpcResponse>(msg);
                    if (!rpc_rsp_msg) {
                        ELOG("rpc响应，向下类型转换失败！");
                        return ;
                    }
                    if (rpc_rsp_msg->rcode() != RCode::RCODE_OK) {
                        ELOG("rpc异步请求出错：%s", errReason(rpc_rsp_msg->rcode()));
                        return ;
                    }
                    result->set_value(rpc_rsp_msg->result());
                }
            private:
                Requestor::ptr _requestor;
        };
    }
}
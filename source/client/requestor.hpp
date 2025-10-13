/*针对所有请求进行处理*/
#include "../common/message.hpp"
#include "../common/net.hpp"
#include<future>
#include<functional>

namespace bitrpc{
    namespace client{
        class Requestor{
            public:
                using ptr = std::shared_ptr<Requestor>;
                using RequestCallback = std::function<void(const BaseMessage::ptr&)>;
                // 用于消费者端，允许线程等待异步任务的结果。
                using AsyncResponse = std::future<BaseMessage::ptr>; // 异步响应
                struct RequestDescribe{
                    using ptr = std::shared_ptr<RequestDescribe>;
                    BaseMessage::ptr request; // 使用指针适合需要在多个地方共享、对象生命周期较长或较大的对象，且需要动态内存分配时。
                    RType rtype;
                    RequestCallback callback; //回调函数回应
                    // 它在一个线程中用于存储一个未来的结果，并将这个结果传递给一个 std::future 对象。
                    std::promise<BaseMessage::ptr> response; // 异步响应
                };
                
                //接收到响应的回调函数
                void onResponse(const BaseConnection::ptr &conn, BaseMessage::ptr &msg)
                {
                    std::string rid = msg->rid();
                    RequestDescribe::ptr rdp = getDescribe(rid);
                    if (rdp.get() == nullptr) {
                        ELOG("收到响应 - %s，但是未找到对应的请求描述！", rid.c_str());
                        return;
                    }
                    // 判断响应调用类型
                    if (rdp->rtype == RType::REQ_ASYNC) { //异步
                        rdp->response.set_value(msg); // 设置响应信息
                    }else if (rdp->rtype == RType::REQ_CALLBACK){ //回调
                        if (rdp->callback) rdp->callback(msg);
                    }else {
                        ELOG("请求类型未知！！");
                    }
                    delDescribe(rid); // 响应完毕后删除该请求，避免重复响应
                }
                // 发送异步请求，并返回 std::future 供调用者获取结果。
                // 使用async_rsp.get() 来阻塞并获取最终结果
                bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, AsyncResponse &async_rsp) {
                    RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_ASYNC); //构造异步请求描述
                    if (rdp.get() == nullptr) {
                        ELOG("构造请求描述对象失败！");
                        return false;
                    }
                    conn->send(req); // 发送响应
                    async_rsp = rdp->response.get_future(); 
                    return true;
                }
                // 发送异步请求，并直接返回同步响应。
                // 调用了第一个版本的 send 函数，并通过 rsp_future.get() 获取异步请求的结果。
                bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, BaseMessage::ptr &rsp) {
                    AsyncResponse rsp_future;
                    bool ret = send(conn, req, rsp_future);
                    if (ret == false) {
                        return false;
                    }
                    rsp = rsp_future.get();
                    return true;
                }
                // 它发送一个带回调函数的请求
                bool send(const BaseConnection::ptr &conn, const BaseMessage::ptr &req, const RequestCallback &cb) {
                    RequestDescribe::ptr rdp = newDescribe(req, RType::REQ_CALLBACK, cb); //构造回调函数请求
                    if (rdp.get() == nullptr) {
                        ELOG("构造请求描述对象失败！");
                        return false;
                    }
                    conn->send(req); // 发送响应
                    return true;
                }

            private: // 增删查
                 RequestDescribe::ptr newDescribe(const BaseMessage::ptr &req, RType rtype, 
                    const RequestCallback &cb = RequestCallback()) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    // 将传入参数保存到RequestDescribe对象中
                    RequestDescribe::ptr rd = std::make_shared<RequestDescribe>();
                    rd->request = req;
                    rd->rtype = rtype;
                    if (rtype == RType::REQ_CALLBACK && cb) { //如果是回调函数，设置回调
                        rd->callback = cb;
                    }//如果是异步操作不在此处处理
                    _request_desc.insert(std::make_pair(req->rid(), rd));
                    return rd;
                }
                RequestDescribe::ptr getDescribe(const std::string &rid) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _request_desc.find(rid);
                    if (it == _request_desc.end()) {
                        return RequestDescribe::ptr();
                    }
                    return it->second;
                }
                void delDescribe(const std::string &rid) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    _request_desc.erase(rid);
                }

            private:
                std::mutex _mutex;
                std::unordered_map<std::string, RequestDescribe::ptr> _request_desc;
        };
    }

}
#include "requestor.hpp"

namespace bitrpc{
    namespace client{
        class RpcCaller{
            public:
                using ptr = std::shared_ptr<RpcCaller>;
                // std::future 的 get() 方法会返回异步操作的结果。对于类型 Json::Value，它会返回 Json::Value 的一个拷贝。
                // 因此，不需要传递引用，因为你并不需要直接访问结果的内存位置，返回的拷贝就是你操作的对象。
                using JsonAsyncResponse = std::future<Json::Value>;
                using JsonResponseCallback = std::function<void(const Json::Value&)>; //注意要加上&
                RpcCaller(const Requestor::ptr &requestor) : _requestor(requestor){}
                // 同步
                void call(const BaseConnection &conn, const std::string &method,
                        const Json::Value &params, Json::Value &result){

                        }
                // 异步
                void call(const BaseConnection &conn, const std::string &method,
                        const Json::Value &params, std::future<Json::Value> &result){

                        }
                //回调
                void call(const BaseConnection &conn, const std::string &method,
                        const Json::Value &params, JsonResponseCallback &result){

                        }
            private:
                Requestor::ptr _requestor;
        };
    }
}
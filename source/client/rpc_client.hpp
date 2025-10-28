/*客户端封装模块：
class RegistryClient：客户端注册对象
    构造：_requestor、_provider、_dispatcher（就是服务提供对象）
    registryMethod ：向外提供的服务注册接口
    参数：_requestor：请求对象、_provider：服务提供对象、_dispatcher：消息分发对象、_client：客户端对象;
class DiscoveryClient：客户端发现对象
*/

#include "../common/dispatcher.hpp"
#include "requestor.hpp"
#include "rpc_caller.hpp"
#include "rpc_registry.hpp"

namespace bitrpc {
    namespace client {
        class RegistryClient {
            public:
                using ptr = std::shared_ptr<RegistryClient>;
                //构造函数传入注册中心的地址信息，用于连接注册中心
                RegistryClient(const std::string &ip, int port)://发送注册请求的前置条件
                    _requestor(std::make_shared<Requestor>()),
                    _provider(std::make_shared<client::Provider>(_requestor)),
                    _dispatcher(std::make_shared<Dispatcher>()) {
                    // 将_requestor绑定Requestor的onResponse，接收到请求响应后调用请求回调
                    auto rsp_cb = std::bind(&client::Requestor::onResponse, _requestor.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    // _dispatcher构造服务请求与其回调的映射
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE, rsp_cb);

                    // 将_dispatcher绑定Dispatcher的onMessage，接收到分发响应后调用分发回调
                    auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _client = ClientFactory::create(ip, port); //构造客户端
                    _client->setMessageCallback(message_cb); //设置客户端发送消息的回调函数（分发）
                    _client->connect();
                }
                //向外提供的服务注册接口
                bool registryMethod(const std::string &method, const Address &host) {
                    return _provider->registryMethod(_client->connection(), method, host);
                }
            private:
                Requestor::ptr _requestor;
                client::Provider::ptr _provider;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;
        };

        class DiscoveryClient {
            public:
                using ptr = std::shared_ptr<DiscoveryClient>;
                //构造函数传入注册中心的地址信息，用于连接注册中心
                DiscoveryClient(const std::string &ip, int port, const Discoverer::OfflineCallback &cb): 
                    _requestor(std::make_shared<Requestor>()),
                    _discoverer(std::make_shared<client::Discoverer>(_requestor, cb)),
                    _dispatcher(std::make_shared<Dispatcher>()){
                    // 将_requestor绑定Requestor的onResponse，接收到请求响应后调用请求回调
                    auto rsp_cb = std::bind(&client::Requestor::onResponse, _requestor.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    // _dispatcher构造服务请求与其回调的映射
                    _dispatcher->registerHandler<BaseMessage>(MType::RSP_SERVICE, rsp_cb);

                    // 将_discoverer绑定Discoverer的onServiceRequest，接收到请求响应后调用请求回调
                    auto req_cb = std::bind(&client::Discoverer::onServiceRequest, _discoverer.get(),
                        std::placeholders::_1, std::placeholders::_2);
                    // _dispatcher构造服务响应与其回调的映射
                    _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE, req_cb);
                    
                    // 将_dispatcher绑定Dispatcher的onMessage，接收到分发响应后调用分发回调
                    auto message_cb = std::bind(&Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _client = ClientFactory::create(ip, port);
                    _client->setMessageCallback(message_cb);
                    _client->connect();
                }
                //向外提供的服务发现接口
                bool serviceDiscovery(const std::string &method, Address &host) {
                    return _discoverer->serviceDiscovery(_client->connection(), method, host);
                }
            private:
                Requestor::ptr _requestor;
                client::Discoverer::ptr _discoverer;
                Dispatcher::ptr _dispatcher;
                BaseClient::ptr _client;
        };

        class RpcClient {
            public:
            private: 
        };

    }
}
/*客户端封装模块：
class RegistryServer ：服务端注册对象
    构造：_requestor、_provider、_dispatcher（就是服务提供对象）
    registryMethod ：向外提供的服务注册接口
    参数：_pd_manager：提供发现管理、_dispatcher、_server：RegistryServer本身;
class RpcServer：RPC对象
    构造：_enableRegistry、_access_addr、_router、_dispatcher
    void registerMethod：注册服务到_reg_client与_router中
    参数：_enableRegistry + _reg_client（注册客户端）、_router（RPCrouter）、dispatcher、_server
        _access_addr：rpc服务提供端地址信息
*/
#include "../common/dispatcher.hpp"
#include "../client/rpc_client.hpp"
#include "rpc_router.hpp"
#include "rpc_registry.hpp"


namespace bitrpc {
    namespace server {
        //注册中心服务端：只需要针对服务注册与发现请求进行处理即可
        class RegistryServer {
            public:
                using ptr = std::shared_ptr<RegistryServer>;
                RegistryServer(int port):
                    _pd_manager(std::make_shared<PDManager>()),
                    _dispatcher(std::make_shared<bitrpc::Dispatcher>())
                {
                    auto service_cb = std::bind(&PDManager::onServiceRequest, _pd_manager.get(),
                        std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<ServiceRequest>(MType::REQ_SERVICE, service_cb);

                    
                    _server = bitrpc::ServerFactory::create(port);
                    auto message_cb = std::bind(&bitrpc::Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _server->setMessageCallback(message_cb);

                    auto close_cb = std::bind(&RegistryServer::onConnShutdown, this, std::placeholders::_1);
                    _server->setCloseCallback(close_cb);
                }
                void start() {
                    _server->start();
                }
            private:
                void onConnShutdown(const BaseConnection::ptr &conn) {
                    _pd_manager->onConnShutdown(conn);
                }
            private:
                PDManager::ptr _pd_manager;
                Dispatcher::ptr _dispatcher;
                BaseServer::ptr _server;
        };

        class RpcServer {
            public:
                using ptr = std::shared_ptr<RpcServer>;
                //rpc——server端有两套地址信息：
                //  1. rpc服务提供端地址信息--必须是rpc服务器对外访问地址（云服务器---监听地址和访问地址不同）
                //  2. 注册中心服务端地址信息 -- 启用服务注册后，连接注册中心进行服务注册用的
                RpcServer(const Address &access_addr, 
                    bool enableRegistry = false, 
                    const Address &registry_server_addr = Address()):
                    _enableRegistry(enableRegistry),
                    _access_addr(access_addr),
                    _router(std::make_shared<bitrpc::server::RpcRouter>()),
                    _dispatcher(std::make_shared<bitrpc::Dispatcher>()) {
                    // 如果启用服务注册
                    if (enableRegistry) {//建立注册中心服务端
                        _reg_client = std::make_shared<client::RegistryClient>(
                            registry_server_addr.first, registry_server_addr.second);
                    }
                    //创建服务提供端_server，并链接rpc请求
                    //当前成员server是一个rpcserver，用于提供rpc服务的
                    auto rpc_cb = std::bind(&RpcRouter::onRequest, _router.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _dispatcher->registerHandler<bitrpc::RpcRequest>(bitrpc::MType::REQ_RPC, rpc_cb);
                    // 使用提供的地址创建 RPC 服务器实例
                    _server = bitrpc::ServerFactory::create(access_addr.second);
                    auto message_cb = std::bind(&bitrpc::Dispatcher::onMessage, _dispatcher.get(), 
                        std::placeholders::_1, std::placeholders::_2);
                    _server->setMessageCallback(message_cb);
                }
                // 注册方法，注册新服务
                void registerMethod(const ServiceDescribe::ptr &service) {
                    //如果开启了服务注册，将本对象的主机地址添加到method方法内。
                    if (_enableRegistry) { 
                        _reg_client->registryMethod(service->method(), _access_addr);
                    }
                    _router->registerMethod(service); //服务管理类注册新服务
                }
                void start() {
                    _server->start();
                }
            private:
                bool _enableRegistry;                 // 是否启用服务注册
                Address _access_addr;                 // RPC服务提供端地址信息（外部访问地址）
                client::RegistryClient::ptr _reg_client;  // 注册客户端，用于服务注册
                RpcRouter::ptr _router;               // 路由器，用于请求路由
                Dispatcher::ptr _dispatcher;           // 调度器，用于处理消息和请求
                BaseServer::ptr _server;              // 服务器实例，用于启动 RPC 服务
        };

// 当启用服务注册时，RPC 服务器在启动时会将自身的服务信息（如服务方法、地址等）注册到 注册中心（一个集中式的服务管理系统）。
// 客户端通过查询注册中心，发现并访问相应的服务，而不需要事先知道服务的具体地址。这样可以实现服务的动态发现和负载均衡。
// 如果服务器地址发生变化，只需要更新注册中心的记录，客户端通过注册中心仍然可以找到服务。

// 如果不启用服务注册功能，RPC 服务器在启动时不会将服务信息注册到任何中心。
// 客户端必须事先知道服务的具体地址和端口，才能直接访问服务。
// 这种方式不支持动态的服务发现，且如果服务地址发生变化，客户端需要手动更新。
        
    }
}
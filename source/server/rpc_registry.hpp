#pragma once

/*管理服务端注册和发现请求
class ProviderManager ：服务提供者管理
    addProvider ：增加提供者
    getProvider ：获取提供者信息（提供者断连）
    delProvider ：删除提供者信息（提供者断连）
    methodHosts ：返回方法对应提供者HOST
class DiscovererManager：发现者管理
    Discoverer::ptr addDiscoverer ：新增发现者
    delDiscoverer ：删除发现者信息（提供者断连）
    onlineNotify ：上线通知（新的提供者上线）
    offlineNotify ：下线通知（提供者下线）
class PDManager ：注册发现管理（外部可连）
    onServiceRequest ：服务请求操作（注册、发现）
    onConnShutdown ：链接断开

*/

#include "../common/message.hpp"
#include "../common/net.hpp"
#include<set>

namespace bitrpc{
    namespace server{
        class ProviderManager{ //服务端（客户端）注册管理
            public:
                //相比make_shared更细节
                using ptr = std::shared_ptr<ProviderManager>;
                struct Provider{ // 服务提供者
                    using ptr = std::shared_ptr<Provider>;
                    BaseConnection::ptr conn; //提供者关联的客户端连接
                    std::mutex _mutex; // 保障methods安全
                    Address host;
                    std::vector<std::string> methods; //请求方法
                    Provider(const BaseConnection::ptr &c, const Address &h):
                        conn(c), host(h){}
                    void appendMethod(const std::string &method) { // 该提供者的对应方法列表添加新方法
                        std::unique_lock<std::mutex> lock(_mutex);
                        //emplace_back: 会原地构造元素，从而避免了不必要的拷贝或移动操作
                        methods.emplace_back(method); 
                    }
                };
                // 当新的服务提供者进行服务注册时使用（新增了提供者）
                void addProvider(const BaseConnection::ptr &c, const Address &h, const std::string &method){
                    //查找连接所关联的服务提供者对象，找到则获取，找不到则创建，并建立关联
                    //Provider::ptr provider:是默认构造的空指针，表示没有关联的对象可用，适合用来表示 "找不到 Provider" 的情况。
                    //std::make_shared: 创建并初始化一个新的 Provider 对象，通常用于需要创建一个实际对象的场景。
                    Provider::ptr provider;
                    {
                        std::unique_lock<std::mutex> lock(_mutex); //创建一个智能锁
                        auto it = _conns.find(c); //查找链接关联的服务对象
                        if(it != _conns.end()) provider = it->second; // 找到了就获取
                        else // 找不到，新建一个服务提供者，并建立联系_conns
                        {
                            provider = std::make_shared<Provider>(c,h); //构建新指针，指向Provider对象
                            _conns.insert(std::make_pair(c, provider)); //插入到_conns中
                        }
                        //method方法的提供主机要多出一个，_providers新增数据
                        auto &providers = _providers[method]; //找到放大对应的提供者set（没有method会自动创建）
                        providers.insert(provider); //这个set再添加一个
                    }
                    provider->appendMethod(method);
                }
                // 当服务提供者断开连接，获取其信息--通知客户服务下线（减少了提供者）
                Provider::ptr getProvider(const BaseConnection::ptr &c){
                    std::unique_lock<std::mutex> lock(_mutex); //创建一个智能锁
                    auto it = _conns.find(c); //查找链接关联的服务对象
                    if(it != _conns.end()) //找到了就返回
                    {
                        return it->second;
                    }
                    return Provider::ptr();    
                }
                // 当服务提供者断开连接，删除其信息
                void delProvider(const BaseConnection::ptr &c)
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(c);
                    if(it == _conns.end())  return ; //找不到：当前断开连接的不是一个服务提供者
                    //如果是提供者，看看提供了什么服务，从服务者提供信息中删除当前服务提供者
                    for(auto &method : it->second->methods)
                    {
                        auto providers = _providers[method];//找到提供该方法对应的提供者列表
                        providers.erase(it->second); //提供者列表中删除该提供者
                    }
                    _conns.erase(it); //删除连接与服务提供者的关联关系
                }
                std::vector<Address> methodHosts(const std::string &method) {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _providers.find(method); //寻找方法的提供者
                    if (it == _providers.end()) {// 没有该方法的提供者
                        return std::vector<Address>(); // 返回空的vector
                    }
                    std::vector<Address> result; // 有该方法，构造address的vector
                    for (auto &provider : it->second) { // 构造result
                        result.push_back(provider->host);
                    }
                    return result;
                }
            private:
                std::mutex _mutex;
                // 存储一个方法有哪些提供者可提供
                std::unordered_map<std::string, std::set<Provider::ptr>> _providers;
                // 存储提供者链接对应的提供者
                std::unordered_map<BaseConnection::ptr, Provider::ptr> _conns;
        };
        class DiscovererManager{ //发现者（客户端）管理（用于客户端进行发现操作）
            public:
                //相比make_shared更细节
                using ptr = std::shared_ptr<DiscovererManager>;
                struct Discoverer{
                    using ptr = std::shared_ptr<Discoverer>;
                    BaseConnection::ptr conn; //发现者关联的客户端连接
                    std::mutex _mutex; //保障methods安全
                    Address host;
                    std::vector<std::string> methods; //发现过的服务名称
                    Discoverer(const BaseConnection::ptr &c):conn(c){} //构造发现者
                    void appendMethod(const std::string &method)
                    {
                        std::unique_lock<std::mutex> _mutex;
                        methods.emplace_back(method);
                    }
                };
                // 当客户端进行服务发现时新增发现者，新增服务名称
                Discoverer::ptr addDiscoverer(const BaseConnection::ptr &c, const std::string &method)
                {
                    Discoverer::ptr discoverer;
                    {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _conns.find(c);//寻找链接对应的discoverer
                        if(it != _conns.end()) //找到对象，不需要新增了
                            discoverer = it->second;
                        else{
                            discoverer = std::make_shared<Discoverer>(c); //构造新发现者
                            _conns.insert(std::make_pair(c, discoverer));//_conns添加链接
                        }
                        //给发现者列表添加该发现者
                        //引用意味着修改discoverers会直接修改_discoverers[method]中的std::set<Discoverer::ptr>
                        auto &discoverers = _discoverers[method];
                        discoverers.insert(discoverer);
                    }
                    discoverer->appendMethod(method); //发现者本身添加方法
                    return discoverer;// 返回发现者
                }
                // 当发现客户端断开连接，删除发现者信息
                void delDiscoverer(const BaseConnection::ptr &c){
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _conns.find(c); //找链接对应发现者
                    if(it == _conns.end()) return ; //没找到
                    for(auto &method : it->second->methods) //删除发现者对应方法
                    {
                        auto discoverers = _discoverers[method]; //找到方法的发现者列表
                        discoverers.erase(it->second); //列表中删除对应发现者
                    }
                    _conns.erase(it);
                }
                // 当新的服务提供者上线，进行上线通知
                void onlineNotify(const std::string &method, const Address &host){
                    return notify(method, host, ServiceOptype::SERVICE_ONLINE);
                }
                // 当一个服务提供者下线，进行下线通知
                void offlineNotify(const std::string &method, const Address &host){
                    return notify(method, host, ServiceOptype::SERVICE_OFFLINE);
                }
            private:
                void notify(const std::string &method, const Address &host, ServiceOptype optype) {
                        std::unique_lock<std::mutex> lock(_mutex);
                        auto it = _discoverers.find(method);
                        if (it == _discoverers.end()) {
                            //这代表这个服务当前没有发现者
                            return;
                        }
                        auto msg_req = MessageFactory::create<ServiceRequest>();
                        msg_req->setID(UUID::uuid());
                        msg_req->setMtype(MType::REQ_SERVICE);
                        msg_req->setMethod(method);
                        msg_req->setHost(host);
                        msg_req->setOptype(optype);
                        for (auto &discoverer : it->second) {
                            discoverer->conn->send(msg_req);
                        }
                    }
            private:
                std::mutex _mutex;
                // 一个方法有哪些发现者
                std::unordered_map<std::string, std::set<Discoverer::ptr>> _discoverers;
                // 存储链接对应的发现者
                std::unordered_map<BaseConnection::ptr, Discoverer::ptr> _conns;
        };

        class PDManager {
            public:
                using ptr =std::shared_ptr<PDManager>;
                PDManager():
                    _providers(std::make_shared<ProviderManager>()),
                    _discoverers(std::make_shared<DiscovererManager>())
                {}
                // 服务请求操作
                void onServiceRequest(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg)
                {
                    //服务操作请求：服务注册/服务发现/
                    ServiceOptype optype = msg->optype(); //获取服务类型
                    if (optype == ServiceOptype::SERVICE_REGISTRY){
                        //服务注册：
                        //  1. 新增服务提供者；  2. 进行服务上线的通知
                        ILOG("%s:%d 注册服务 %s", msg->host().first.c_str(), msg->host().second, msg->method().c_str());
                        _providers->addProvider(conn, msg->host(), msg->method()); //方法添加提供者
                        _discoverers->onlineNotify(msg->method(), msg->host()); // 给方法订阅发现者上线通知
                        return registryResponse(conn, msg); //发送注册响应
                    } else if (optype == ServiceOptype::SERVICE_DISCOVERY){
                        //服务发现：
                        //  1. 新增服务发现者
                        ILOG("客户端要进行 %s 服务发现！", msg->method().c_str());
                        _discoverers->addDiscoverer(conn, msg->method()); // 添加对某method的发现者
                        return discoveryResponse(conn, msg);//发送发现响应
                    }else {
                        ELOG("收到服务操作请求，但是操作类型错误！");
                        return errorResponse(conn, msg); //发送错误响应
                    }
                }
                void onConnShutdown(const BaseConnection::ptr &conn){ //连接断开
                    auto provider = _providers->getProvider(conn); // 获取conn对应的提供者
                    if (provider.get() != nullptr) {//如果时提供者
                        ILOG("%s:%d 服务下线", provider->host.first.c_str(), provider->host.second);
                        for (auto &method : provider->methods) { //找到该提供者提供的方法列表
                            _discoverers->offlineNotify(method, provider->host); // 通知订阅方法的发现者那个提供者下线了
                        }
                        _providers->delProvider(conn); // 删除该链接对应提供者，直接返回
                    }
                    _discoverers->delDiscoverer(conn); //如果是发现者，直接删除发现者即可，不需要通知
                }
            private:
                 void errorResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg) {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setID(msg->rid());
                    msg_rsp->setMtype(MType::RSP_SERVICE);
                    msg_rsp->setRCode(RCode::RCODE_INVALID_OPTYPE);
                    msg_rsp->setOptype(ServiceOptype::SERVICE_UNKNOW); //未知服务
                    conn->send(msg_rsp);
                }
                //服务注册响应
                void registryResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg) {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setID(msg->rid());
                    msg_rsp->setMtype(MType::RSP_SERVICE);
                    msg_rsp->setRCode(RCode::RCODE_OK);
                    msg_rsp->setOptype(ServiceOptype::SERVICE_REGISTRY);
                    conn->send(msg_rsp);
                }
                void discoveryResponse(const BaseConnection::ptr &conn, const ServiceRequest::ptr &msg) {
                    auto msg_rsp = MessageFactory::create<ServiceResponse>();
                    msg_rsp->setID(msg->rid());
                    msg_rsp->setMtype(MType::RSP_SERVICE);
                    msg_rsp->setOptype(ServiceOptype::SERVICE_DISCOVERY);
                    // 获取提供者信息
                    std::vector<Address> hosts = _providers->methodHosts(msg->method());
                    if (hosts.empty()) { // 如果无人提供
                        msg_rsp->setRCode(RCode::RCODE_NOT_FOUND_SERVICE); //设置CODE为错误
                        return conn->send(msg_rsp);
                    }   
                    // 有人提供
                    msg_rsp->setRCode(RCode::RCODE_OK);
                    msg_rsp->setMethod(msg->method());
                    msg_rsp->setHost(hosts);
                    return conn->send(msg_rsp);
                }
            private:
                ProviderManager::ptr _providers;
                DiscovererManager::ptr _discoverers;
        };
    }
}
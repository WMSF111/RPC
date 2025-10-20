#pragma once

/*管理服务端注册和发现请求

*/

#include "../common/message.hpp"
#include "../common/net.hpp"

namespace bitrpc{
    namespace server{
        class ProviderManager{ //服务端（客户端）注册管理
            public:
                //相比make_shared更细节
                using ptr = std::shared_ptr<ProviderManager>;
                struct Provider{
                    using ptr = std::shared_ptr<Provider>;
                    BaseConnection::ptr conn; //提供者关联的客户端连接
                    std::mutex _mutex; // 保障methods安全
                    Address host;
                    std::vector<std::string> methods; //请求方法
                    Provider(const BaseConnection::ptr &c, const Address &h);
                    void appendMethod(const std::string &methon);
                };
                // 当新的服务提供者进行服务注册时使用（新增了提供者）
                Provider::ptr addProvider(const BaseConnection::ptr &c, const Address &h);
                // 当服务提供者断开连接，获取其信息--通知客户服务下线（减少了提供者）
                Provider::ptr getProvider(const BaseConnection::ptr &c);
                // 当服务提供者断开连接，删除其信息
                void delProvider(const BaseConnection::ptr &c);
            private:
                std::mutex _mutex;
                // 存储一个方法有哪些提供者可提供
                std::unordered_map<std::string, std::vector<Provider::ptr>> _providers;
                // 存储提供者链接对应的提供者
                std::unordered_map<BaseConnection::ptr, Provider::ptr> _conns;
        };
        class DiscovererManager{ //发现者（客户端）管理（用于客户端进行发现操作）
            public:
                //相比make_shared更细节
                using ptr = std::shared_ptr<DiscovererManager>;
                struct Discover{
                    using ptr = std::shared_ptr<Discover>;
                    BaseConnection::ptr conn; //发现者关联的客户端连接
                    std::mutex _mutex; //保障methods安全
                    Address host;
                    std::vector<std::string> methods; //发现过的服务名称
                    Discover(const BaseConnection::ptr &c); //构造发现者
                    void appendMethod(const std::string &methon);
                };
                // 当客户端进行服务发现时新增发现者，新增服务名称
                Discover::ptr addDiscover(const BaseConnection::ptr &c);
                // 当发现客户端断开连接，删除发现者信息
                void delProvider(const BaseConnection::ptr &c);
                // 当新的服务提供者上线，进行上线通知
                void onlineNotify(const std::string &method);
                // 当一个服务提供者下线，进行下线通知
                void offlineNotify(const std::string &method);
            private:
                std::mutex _mutex;
                // 一个方法有哪些发现者
                std::unordered_map<std::string, std::vector<Discover::ptr>> _providers;
                // 存储链接对应的发现者
                std::unordered_map<BaseConnection::ptr, Discover::ptr> _conns;
        };

        class PDManager {
            public:
                using ptr =std::shared_ptr<PDManager>;
                void onServiceRequest(const BaseConnection::ptr &conn,const ServiceRequest::ptr &msg);
                void onConnShutdown(const BaseConnection::ptr &conn);
            private:
                ProviderManager::ptr providers;
                DiscovererManager::ptr discoverers;
        };
    }
}
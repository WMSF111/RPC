#include "../common/message.hpp"
#include "../common/net.hpp"

namespace bitrpc{
    namespace server{
        enum class VType {
            BOOL = 0,
            INTEGRAL,
            NUMERIC,
            STRING,
            ARRAY,
            OBJECT,
        };

        class ServiceDescribe{ //服务描述
            public:
                using ptr = std::shared_ptr<ServiceDescribe>;
                // 传入：请求和响应
                using ServiceCallback = std::function<void(const Json::Value&, Json::Value &)>; //回调函数
                using ParamsDescribe = std::pair<std::string, VType>; //用于确定方法对应的类型
                ServiceDescribe(std::string &&mname, std::vector<ParamsDescribe> &&desc, 
                    VType vtype, ServiceCallback &&handler) : 
                    _method_name(std::move(mname)),_callback(std::move(handler)), 
                    _params_desc(std::move(desc)), _return_type(vtype)
                {} // 构造：需要传入方法名称
                bool ParamsChek(const Json::Value &params){ }//用于检查参数类型是否正确
            private:
                std::string _method_name;   // 方法名称
                ServiceCallback _callback;  // 实际的业务回调函数
                std::vector<ParamsDescribe> _params_desc; // 参数字段格式描述
                VType _return_type; //结果作为返回值类型的描述
        };

        class SDescribeFactory { // 外部能够构造服务描述
            public:
                ServiceDescribe::ptr build() {// 生产服务描述

                }
            private:

        };

        class ServiceManage{ // 服务管理类
            public:
                using ptr = std::shared_ptr<ServiceManage>;
                ServiceDescribe::ptr create(){} // 增
                ServiceDescribe::ptr select(){} // 查
                void remove(){} // 删
            private:
                std::mutex _mutex;
                std::unordered_map<std::string, ServiceDescribe::ptr> _services;// 提供的服务描述
        };

        class RpcRouter{  // 用于跟其他模块耦合的模块
            //服务注册、rpc请求回调函数
            public:
                using ptr = std::shared_ptr<RpcRouter>;
            //使用智能指针：避免拷贝、管理资源、引用计数
            //使用引用&：避免拷贝、允许你在函数内修改指针的值，而直接传递指针会使得无法修改原指针
                void onRequest(const BaseConnection::ptr &conn, RpcRequest::ptr &request){
                    //1.查询客户端请求方法描述，判断服务端是否能提供服务
                    //2.参数校验，确定是否能提供服务
                    //3.调用业务回调函数进行处理
                    //4.处理完毕得到结果，组织响应，向客户端发送
                } //用于提供rpc请求回调

                void registerMethod(const ServiceDescribe::ptr &service){// 服务注册
                    // 需要传入方法的参数描述， 选择用参数描述对象更方便
                }
            private:
                ServiceManage::ptr _service_manager; //服务管理
        };
    }
}
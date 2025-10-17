#pragma once

/*提供服务端rpc请求
1. enum class VType ：提供的参数类型
2. ServiceDescribe ：描述一个服务的基本信息（方法名、参数类型、回调函数等）以及参数和返回值的校验。
                    ServiceCallback 业务回调函数
                    vector<ParamsDescribe>：保存参数描述
3. SDescribeFactory ：构造服务描述的工厂类，方便创建服务描述对象。
                    vector<ServiceDescribe::ParamsDescribe>，用于构造ServiceDescribe
4. ServiceManager ：服务管理类，负责注册、查询和删除服务。构造map映射ServiceDescribe::ptr
5. RpcRouter ：处理客户端请求的路由类，负责服务的查找、参数校验、回调处理和响应发送。ServiceManager是其私有参数
*/

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
                //定义一个智能指针
                using ptr = std::shared_ptr<ServiceDescribe>;
                // 传入：请求和响应
                using ServiceCallback = std::function<void(const Json::Value&, Json::Value &)>; //回调函数
                using ParamsDescribe = std::pair<std::string, VType>; //用于确定方法对应的类型
                // 右值属性会产生内部资源交换不能使用const
                ServiceDescribe(std::string &&mname, std::vector<ParamsDescribe> &&desc, 
                    VType vtype, ServiceCallback &&handler) : 
                    _method_name(std::move(mname)),_callback(std::move(handler)), 
                    _params_desc(std::move(desc)), _return_type(vtype)
                {} // 构造：需要传入方法名称
                const std::string &method(){ return _method_name;}
                ///针对收到的请求中的参数进行校验
                bool ParamsCheck(const Json::Value &params){//用于检查参数类型是否正确
                //对params进行参数校验---判断所描述的参数字段是否存在，类型是否一致
                    for(auto &desc : _params_desc)
                    {
                        if(params.isMember(desc.first) == false) //判断对象是否存在
                        {
                            ELOG("参数字段完整性校验失败！%s 字段缺失！", desc.first.c_str());
                            return false;
                        }
                        if(check(desc.second, params[desc.first]) == false) //判断对象的类型是否正确
                        {
                            ELOG("参数字段完整性校验失败！%s 字段缺失！", desc.first.c_str());
                            return false;
                        }
                    }
                    return true;
                 }
                 bool callbackcheck(const Json::Value& val, Json::Value & result)
                 {
                    _callback(val, result);
                    if(rtcheck(result) == false)
                    {
                        ELOG("回调处理函数中的响应信息校验失败！");
                        return false;
                    }
                    return true;
                 }
            private:
                bool rtcheck(const Json::Value &val)
                {
                    return check(_return_type, val);
                }
                bool check(const VType &vtype, const Json::Value &val)
                {
                    switch(vtype){
                        case VType::BOOL : return val.isBool(); // bool类型
                        case VType::INTEGRAL : return val.isIntegral(); //整数类型
                        case VType::NUMERIC : return val.isNumeric(); //浮点数类型
                        case VType::STRING : return val.isString(); // 字符串类型
                        case VType::ARRAY : return val.isArray(); // 数组类型
                        case VType::OBJECT : return val.isObject(); // 对象类型
                    }
                    return false;
                }
                std::string _method_name;   // 方法名称
                ServiceCallback _callback;  // 实际的业务回调函数
                std::vector<ParamsDescribe> _params_desc; // 参数字段格式描述
                VType _return_type; //结果作为返回值类型的描述
        };

        class SDescribeFactory { // 外部能够构造服务描述
            public:
                void setMethodName(const std::string &method)
                {
                    _method_name = method;
                }
                void setParamsDesc(const std::string &pname, VType vtype)
                {
                    _params_desc.push_back(ServiceDescribe::ParamsDescribe(pname, vtype));
                }
                void setCallback(const ServiceDescribe::ServiceCallback &cb) {
                    _callback = cb;
                }
                void setReturnType(const VType &type)
                {
                    _return_type = type;
                }
                ServiceDescribe::ptr build() {// 生产服务描述
                // 要构造智能指针方便管理
                // _method_name是一个左值（即一个已经存在的对象）,std::move将它转化为右值引用.
                // 表示我们不再需要这个原始的 _method_name 对象，并且它的资源可以被转移（而非拷贝）。
                    return std::make_shared<ServiceDescribe>(std::move(_method_name), 
                        std::move(_params_desc), _return_type, std::move(_callback));
                }
            private:
                std::string _method_name;   // 方法名称
                ServiceDescribe::ServiceCallback _callback;  // 实际的业务回调函数
                std::vector<ServiceDescribe::ParamsDescribe> _params_desc; // 参数字段格式描述
                VType _return_type; //结果作为返回值类型的描述
        };

        class ServiceManager{ // 服务管理类
            public:
                using ptr = std::shared_ptr<ServiceManager>;
                void insert(const ServiceDescribe::ptr &desc){// 增
                    std::unique_lock<std::mutex> lock(_mutex);
                    _services.insert(std::make_pair(desc->method(), desc));
                } 
                ServiceDescribe::ptr select(const std::string &method_name){// 查
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _services.find(method_name);
                    if(it == _services.end())
                    {
                        ELOG("找不到Method对应的Service！");
                        return ServiceDescribe::ptr();
                    }
                    return it->second;
                } 
                void remove(const std::string &method_name){// 删
                    std::unique_lock<std::mutex> lock(_mutex);
                    _services.erase(method_name);
                } 
            private:
                std::mutex _mutex;
                std::unordered_map<std::string, ServiceDescribe::ptr> _services;// 提供的服务描述
        };

        class RpcRouter{  // 用于跟其他模块耦合的模块
            //服务注册、rpc请求回调函数
            public:
                using ptr = std::shared_ptr<RpcRouter>;
                RpcRouter(): _service_manager(std::make_shared<ServiceManager>()){}
            //使用智能指针：避免拷贝、管理资源、引用计数
            //使用引用&：避免拷贝、允许你在函数内修改指针的值，而直接传递指针会使得无法修改原指针
                void onRequest(const BaseConnection::ptr &conn, RpcRequest::ptr &request){
                    //1.查询客户端请求方法描述，判断服务端是否能提供服务
                    auto service = _service_manager->select(request->method()); //查
                    if (service.get() == nullptr) {
                        ELOG("%s 服务未找到！", request->method().c_str());
                        // 不仅后端需要看到错误，还需要做出响应
                        return response(conn, request, Json::Value(), RCode::RCODE_NOT_FOUND_SERVICE);
                    }
                    //2.参数校验，确定是否能提供服务
                    if (service->ParamsCheck(request->params()) == false) {
                        ELOG("%s 参数校验失败！ ", request->method().c_str());
                        return response(conn, request, Json::Value(), RCode::RCODE_INVALID_PARAMS);
                    }
                    //3.调用业务回调函数进行处理
                    Json::Value result;
                    bool ret = service->callbackcheck(request->params(), result);
                    if (ret == false) {
                        ELOG("%s 服务参数校验失败！", request->method().c_str());
                        return response(conn, request, Json::Value(), RCode::RCODE_INTERNAL_ERROR);
                    }
                    //4.处理完毕得到结果，组织响应，向客户端发送
                    return response(conn, request, result, RCode::RCODE_OK);
                } //用于提供rpc请求回调

                void registerMethod(const ServiceDescribe::ptr &service){// 服务注册
                    // 需要传入方法的参数描述， 选择用参数描述对象更方便
                    _service_manager->insert(service);
                }
            private:
                void response(const BaseConnection::ptr &conn,  // 响应发送
                    const RpcRequest::ptr &req, 
                    const Json::Value &res, RCode rcode) {
                    auto msg = MessageFactory::create<RpcResponse>();
                    msg->setID(req->rid());
                    msg->setMtype(bitrpc::MType::RSP_RPC);
                    msg->setRCode(rcode);
                    msg->setResult(res);
                    conn->send(msg);
                }
            private:
                ServiceManager::ptr _service_manager; //服务管理
        };
    }
}
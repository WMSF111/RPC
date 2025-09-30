#pragma once
#include"detail.hpp"
#include"fieIds.hpp"
#include"abstract.hpp"

namespace bitrpc{
    typedef std::pair<std::string, int> Address;
    class MessageJson : public BaseMessage{ // Message类
        public:
            using ptr = std::shared_ptr<MessageJson>;
            virtual std::string serialize() override{ // 检查格式是否合格
                std::string body;
                bool ret = JsonUtil::serialize(_body, body);
                if(ret == false) return std::string(); //返回一个空的字符串
                return body;
            } 
            virtual bool unserialize(const std::string &msg) override{
                return JsonUtil::unserialize(_body, msg); 
            }; 
        protected:
            Json::Value _body;
    };

    class JsonRequest : public MessageJson{ // Json请求
        public:
            using ptr = std::shared_ptr<JsonRequest>; 
    };

    class JsonResponse : public MessageJson{ //Json响应，需要检查rcode响应状态码
        public:
            using ptr = std::shared_ptr<JsonResponse>;
             virtual bool check() override {
                //在响应中，大部分的响应都只有响应状态码
                //因此只需要判断响应状态码字段是否存在，类型是否正确即可
                if (_body[KEY_RCODE].isNull() == true) {
                    ELOG("响应中没有响应状态码！");
                    return false;
                }
                if (_body[KEY_RCODE].isIntegral() == false) {
                    ELOG("响应状态码类型错误！");
                    return false;
                }
                return true;
            }
            virtual RCode rcode() {
                return (RCode)_body[KEY_RCODE].asInt();
            }
            virtual void setRCode(RCode rcode) {
                _body[KEY_RCODE] = (int)rcode;
            }
    };

    //Rpc请求，需要METHOD、PARAMS
    class RpcRequest : public JsonRequest{ 
        public:
            using ptr = std::shared_ptr<RpcRequest>;
            virtual bool check() override {
                // rpc请求中，包含请求方法名称-字符串，参数字段-对象
                if (_body[KEY_METHOD].isNull() == true ||
                    _body[KEY_METHOD].isString() == false) {
                    ELOG("RPC请求中没有方法名称或方法名称类型错误！");
                    return false;
                }
                if (_body[KEY_PARAMS].isNull() == true ||
                    _body[KEY_PARAMS].isObject() == false) {
                    ELOG("RPC请求中没有参数信息或参数信息类型错误！");
                    return false;
                }
                return true;
            }
            std::string method() {
                return _body[KEY_METHOD].asString();
            }
            void setMethod(const std::string &method_name) {
                _body[KEY_METHOD] = method_name;
            }
            Json::Value params() {
                return _body[KEY_PARAMS];
            }
            void setParams(const Json::Value &params) {
                _body[KEY_PARAMS] = params;
            }
    };

    // 主题请求，主题发布
    // 需要TOPIC、OPTYPE、TOPIC_MSG（只有主题发布需要）
    class TopicRequest : public JsonRequest { 
        public:
            using ptr = std::shared_ptr<TopicRequest>;
            virtual bool check() override {
                //rpc请求中，包含请求方法名称-字符串，参数字段-对象
                if (_body[KEY_TOPIC_KEY].isNull() == true ||
                    _body[KEY_TOPIC_KEY].isString() == false) {
                    ELOG("主题请求中没有主题名称或主题名称类型错误！");
                    return false;
                }
                if (_body[KEY_OPTYPE].isNull() == true ||
                    _body[KEY_OPTYPE].isIntegral() == false) {
                    ELOG("主题请求中没有操作类型或操作类型的类型错误！");
                    return false;
                }
                if (_body[KEY_OPTYPE].asInt() == (int)TopicOptype::TOPIC_PUBLISH &&
                    (_body[KEY_TOPIC_MSG].isNull() == true ||
                    _body[KEY_TOPIC_MSG].isString() == false)) {
                    ELOG("主题消息发布请求中没有消息内容字段或消息内容类型错误！");
                    return false;
                }
                return true;
            }
            
            std::string topicKey() {
                return _body[KEY_TOPIC_KEY].asString();
            }
            void setTopicKey(const std::string &key) {
                _body[KEY_TOPIC_KEY] = key;
            }
            TopicOptype optype() {
                return (TopicOptype)_body[KEY_OPTYPE].asInt();
            }
            void setOptype(TopicOptype optype) {
                _body[KEY_OPTYPE] = (int)optype;
            }
            std::string topicMsg() {
                return _body[KEY_TOPIC_MSG].asString();
            }
            void setTopicMsg(const std::string &msg) {
                _body[KEY_TOPIC_MSG] = msg;
            }

    };
    
    // 服务请求，服务注册、服务发现、服务上线、服务下线
    // 需要METHOD、OPTYPE、HOST（IP、PORT，只有SERVICE_DISCOVERY不需要， 因为请求时不知道有哪些service）
    class ServiceRequest : public JsonRequest { 
        public:
            using ptr = std::shared_ptr<ServiceRequest>;
            virtual bool check() override {
                //rpc请求中，包含请求方法名称-字符串，参数字段-对象
                if (_body[KEY_METHOD].isNull() == true ||
                    _body[KEY_METHOD].isString() == false) {
                    ELOG("服务请求中没有方法名称或方法名称类型错误！");
                    return false;
                }
                if (_body[KEY_OPTYPE].isNull() == true ||
                    _body[KEY_OPTYPE].isIntegral() == false) {
                    ELOG("服务请求中没有操作类型或操作类型的类型错误！");
                    return false;
                }
                if (_body[KEY_OPTYPE].asInt() != (int)(ServiceOptype::SERVICE_DISCOVERY) &&
                    (_body[KEY_HOST].isNull() == true ||
                    _body[KEY_HOST].isObject() == false ||
                    _body[KEY_HOST][KEY_HOST_IP].isNull() == true ||
                    _body[KEY_HOST][KEY_HOST_IP].isString() == false ||
                    _body[KEY_HOST][KEY_HOST_PORT].isNull() == true ||
                    _body[KEY_HOST][KEY_HOST_PORT].isIntegral() == false)) {
                    ELOG("服务请求中主机地址信息错误！");
                    return false;
                }
                return true;
            }
            
            std::string method() {
                return _body[KEY_METHOD].asString();
            }
            void setMethod(const std::string &name) {
                _body[KEY_METHOD] = name;
            }
            ServiceOptype optype() { // 主题操作类型,ServiceOptype，用于代指不同操作
                return (ServiceOptype)_body[KEY_OPTYPE].asInt();
            }
            void setOptype(ServiceOptype optype) {
                _body[KEY_OPTYPE] = (int)optype;
            }
            Address host() { // 获取pair的Address
                Address addr;
                addr.first = _body[KEY_HOST][KEY_HOST_IP].asString();
                addr.second = _body[KEY_HOST][KEY_HOST_PORT].asInt();
                return addr;
            }
            void setHost(const Address &host) { // 将pair的Address赋值给HOST
                Json::Value val;
                val[KEY_HOST_IP] = host.first;
                val[KEY_HOST_PORT] = host.second;
                _body[KEY_HOST] = val;
            }
    };
    
    // Rpc响应，返回响应结果。
    // 需要RESULT
    class RpcResponse : public JsonResponse { 
        public:
            using ptr = std::shared_ptr<RpcResponse>;
            virtual bool check() override {
                if (_body[KEY_RESULT].isNull() == true) {
                    ELOG("响应中没有Rpc调用结果,或结果类型错误！");
                    return false;
                }
                return true;
            }
            Json::Value result() {
                return _body[KEY_RESULT];
            }
            void setResult(const Json::Value &result) {
                _body[KEY_RESULT] = result;
            }
    };

    class TopicResponse : public JsonResponse { // 主题响应，包含RCODE
        public:
            using ptr = std::shared_ptr<TopicResponse>;
    };

    class ServiceResponse : public JsonResponse { //服务响应，包含RCODE、OPTYPE、METHOD、HOST
    // 注册、上线、下线只需要RCODE, 服务发现都需要
    // OPTYPE用于看响应类型
        public:
            using ptr = std::shared_ptr<ServiceResponse>;
            virtual bool check() override {
                if (_body[KEY_OPTYPE].isNull() == true ||
                    _body[KEY_OPTYPE].isIntegral() == false) {
                    ELOG("响应中没有操作类型,或操作类型的类型错误！");
                    return false;
                }
                if (_body[KEY_OPTYPE].asInt() == (int)(ServiceOptype::SERVICE_DISCOVERY) &&
                   (_body[KEY_METHOD].isNull() == true ||
                    _body[KEY_METHOD].isString() == false ||
                    _body[KEY_HOST].isNull() == true ||
                    _body[KEY_HOST].isArray() == false)) {
                    ELOG("服务发现响应中响应信息字段错误！");
                    return false;
                }
                return true;
            }
            ServiceOptype optype() {
                return (ServiceOptype)_body[KEY_OPTYPE].asInt();
            }
            void setOptype(ServiceOptype optype) {
                _body[KEY_OPTYPE] = (int)optype;
            }
            std::string method() {
                return _body[KEY_METHOD].asString();
            }
            void setMethod(const std::string &method) {
                _body[KEY_METHOD] = method;
            }
            //因为ServiceResponse可能会有多个服务器
            void setHost(std::vector<Address> addrs) {
                for (auto &addr : addrs) { // 循环设置HOST
                    Json::Value val;
                    val[KEY_HOST_IP] = addr.first;
                    val[KEY_HOST_PORT] = addr.second;
                    _body[KEY_HOST].append(val);
                }
            }
            std::vector<Address> hosts() { //循环获取装着pair的Addres的vector
                std::vector<Address> addrs;
                int sz = _body[KEY_HOST].size(); 
                for (int i = 0; i < sz; i++) {
                    Address addr;
                    addr.first = _body[KEY_HOST][i][KEY_HOST_IP].asString();
                    addr.second = _body[KEY_HOST][i][KEY_HOST_PORT].asInt();
                    addrs.push_back(addr);
                }
                return addrs;
            }
    };

    // 消息对象工厂
    // 提供了一个集中管理对象创建的地方，避免了代码中到处硬编码对象创建逻辑的麻烦。
    class MessageFactory{
        public:
        // 根据MType类型创建对应对象
            static BaseMessage::ptr create(MType mtype){
                switch(mtype) {
                    // 返回一个mtype类型对象， 但用BaseMessage::ptr指针接受
                    //后续还需要转化为对应派生类指针
                    case MType::REQ_RPC : return std::make_shared<RpcRequest>();
                    case MType::RSP_RPC : return std::make_shared<RpcResponse>();
                    case MType::REQ_TOPIC : return std::make_shared<TopicRequest>();
                    case MType::RSP_TOPIC : return std::make_shared<TopicResponse>();
                    case MType::REQ_SERVICE : return std::make_shared<ServiceRequest>();
                    case MType::RSP_SERVICE : return std::make_shared<ServiceResponse>();
                }
                return BaseMessage::ptr();
            }
            // 当后期需要传入参数时，可以用该方法创造对象
            // 根据具体类型T创建具体对象T
            template<typename T, typename ...Args>
            // 不能返回BaseMessage::ptr类型，因为父类指针无法转化为子类指针（子类指针不能指向父类对象）
            // std::dynamic pointer cast<bitrpc::RpcRequest>(bmp);可以转换
            static std::shared_ptr<T> create(Args&& ...args) {
                return std::make_shared<T>(std::forward(args)...);
            }
        };

}

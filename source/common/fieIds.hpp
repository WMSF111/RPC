#pragma once
#include<iostream>
#include<string>
#include<unordered_map>


namespace bitrpc{
    #define KEY_METHOD "method"
    #define KEY_PARAMS "parameters"
    #define KEY_TOPIC_KEY "topic_key"
    #define KEY_TOPIC_MSG "topic_msg"
    #define KEY_OPTYPE "optype"
    #define KEY_HOST "host"
    #define KEY_HOST_IP "ip"
    #define KEY_HOST_PORT "port"
    #define KEY_RCODE "rcode"
    #define KEY_RESULT "result"

    enum class MType {
        REQ_RPC = 0,
        RSP_RPC,
        REQ_TOPIC,
        RSP_TOPIC,
        REQ_SERVICE,
        RSP_SERVICE
    };  

    enum class RCode { // 响应码类型定义 
        RCODE_OK = 0,
        RCODE_PARSE_FAILED,
        RCODE_ERROR_MSGTYPE,
        RCODE_INVALID_MSG,
        RCODE_DISCONNECTED,
        RCODE_INVALID_PARAMS,
        RCODE_NOT_FOUND_SERVICE,
        RCODE_INVALID_OPTYPE,
        RCODE_NOT_FOUND_TOPIC,
        RCODE_INTERNAL_ERROR
    };

    static std::string errReason(RCode code) {
        static std::unordered_map<RCode, std::string> err_map = { // 保存不同RCode对应错误原因
            {RCode::RCODE_OK, "成功处理！"},
            {RCode::RCODE_PARSE_FAILED, "消息解析失败！"},
            {RCode::RCODE_ERROR_MSGTYPE, "消息类型错误！"},
            {RCode::RCODE_INVALID_MSG, "⽆效消息"},
            {RCode::RCODE_DISCONNECTED, "连接已断开！"},
            {RCode::RCODE_INVALID_PARAMS, "⽆效的Rpc参数！"},
            {RCode::RCODE_NOT_FOUND_SERVICE, "没有找到对应的服务！"},
            {RCode::RCODE_INVALID_OPTYPE, "⽆效的操作类型"},
            {RCode::RCODE_NOT_FOUND_TOPIC, "没有找到对应的主题！"},
            {RCode::RCODE_INTERNAL_ERROR, "内部错误！"}
            };
        auto it = err_map.find(code); // 查找有没有对应错误
        if (it == err_map.end()) return "未知错误！";
        return it->second;
    }
    
    enum class RType { //RPC请求类型定义 
        REQ_ASYNC = 0, // 异步请求：返回异步对象，在需要的时候通过异步对象获取响应结果（还未收到结果会阻塞）
        REQ_CALLBACK // 回调请求：设置回调函数，通过回调函数对响应进⾏处理
    };

    enum class TopicOptype { // 主题操作类型定义
    TOPIC_CREATE = 0, // 主题创建
    TOPIC_REMOVE, // 主题删除
    TOPIC_SUBSCRIBE, // 主题订阅
    TOPIC_CANCEL, // 主题取消订阅
    TOPIC_PUBLISH // 主题消息发布
    };

    enum class ServiceOptype { // 服务操作类型定义
        SERVICE_REGISTRY = 0, // 服务注册
        SERVICE_DISCOVERY, // 服务发现
        SERVICE_ONLINE, // 服务上线
        SERVICE_OFFLINE, // 服务下线
        SERVICE_UNKNOW
    };
}
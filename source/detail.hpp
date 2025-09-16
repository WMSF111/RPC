/*
    实现项目中用到的一些琐碎功能代码
    1.日志宏的定义
    2.json的序列化和反序列化
    3.uuid的生成
*/
#pragma once
#include<cstdio>
#include<ctime>
#include<iostream>
#include<string>
#include <sstream>
#include<jsoncpp/json/json.h>

namespace bitspace
{

#define LDBG 0
#define LINF 1
#define LERR 2

#define LDEFAULT LINF

#define LOG(level, formt, ...) {\
    if(level >= LDEFAULT){\
        time_t t = time(NULL); \
        struct tm* lt = localtime(&t); \
        char temp[32] = {0};\
        strftime(temp, 31,"%m-%d %T" , lt);\
        printf("[%s][%s %d] " formt "\n", temp, __FILE__, __LINE__, ##__VA_ARGS__);\
    }\
}//宏定义后面带##是为了可以省略该逗号及后面的参数

//为每种等级建立不同日志，方便后期显示
#define DLOG(formt, ...) LOG(LDBG, formt, ##__VA_ARGS__)
#define ILOG(formt, ...) LOG(LINF, formt, ##__VA_ARGS__)
#define ELOG(formt, ...) LOG(LERR, formt, ##__VA_ARGS__)

    class JsonUtil
    {
    public: // static可以让不需要初始化就使用该函数
        /*输入：JsonValue类，要转化的string类；
        目的：将JsonValue其转化为string类
        返回：转化成功与否*/
        static bool serialize(Json::Value &val, std::string &body) // 序列化函数
        {
            Json::StreamWriterBuilder swb;
            swb["emitUTF8"] = true;
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter()); // 创建新的StreamWriter类
            std::stringstream ss; // 转换成字节流格式，方便传给write。
            int ret = sw->write(val, &ss); //读取JSONValue类val给ss
            if(ret != 0)
            {
                ELOG("Json serialize failed.\n");
                return false;
            }
            body = ss.str(); // 转换为string格式，方便调用输出
            return true;
        }

        /*输入：JsonValue类，要转化的string类；
        目的：将string类转化为JsonValue类
        返回：转化成功与否*/
        static bool unserialize(Json::Value &val, std::string &body)
        {
            Json::CharReaderBuilder crb;
            std::string err;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
            // c_str：转换成 C 风格的字符串（即以null字符\0结尾的字符数组）
            int ret = cr->parse(body.c_str(),body.c_str() + body.size(), &val, &err);
            if(ret == false) // 有错误
            {
                ELOG("Json unserialize failed: %s \n" , err);
                return false;
            }
            return true;
        }
    };
}
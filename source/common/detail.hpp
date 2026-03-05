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
#include <chrono>
#include <random>
#include <atomic>
#include <iomanip>

namespace bitrpc // 日志宏的定义
{

#define LDBG 0 // 调试信息，开发阶段使用
#define LINF 1 // 一般信息，正常运行日志
#define LERR 2 // 错误信息，异常或故障记录

#define LDEFAULT LDBG //控制输出阈值（如设为 1，则只输出 Info 及以上级别）

#define LOG(level, format, ...) {\
    if(level >= LDEFAULT){\
        time_t t = time(NULL); \
        struct tm* lt = localtime(&t); \
        char time_tmp[32] = {0};\
        strftime(time_tmp, 31,"%m-%d %T" , lt);\
        fprintf(stdout, "[%s][%s:%d] " format "\n", time_tmp, __FILE__, __LINE__, ##__VA_ARGS__);\
    }\
}//宏定义后面带##是为了可以省略该逗号及后面的参数

//为每种等级建立不同日志，方便后期显示
#define DLOG(formt, ...) LOG(LDBG, formt, ##__VA_ARGS__);
#define ILOG(formt, ...) LOG(LINF, formt, ##__VA_ARGS__);
#define ELOG(formt, ...) LOG(LERR, formt, ##__VA_ARGS__);


    class JsonUtil // 封装 JSON 数据的序列化（对象→字符串）和反序列化（字符串→对象）操作。
    {
    public: // static可以让不需要初始化就使用该函数
        /*输入：JsonValue类
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

        /*输入：string类
        目的：将string类转化为JsonValue类
        返回：转化成功与否*/
        static bool unserialize(Json::Value &val, const std::string &body) // body需要用const
        {
            Json::CharReaderBuilder crb;
            std::string err;
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
            // c_str：转换成 C 风格的字符串（即以null字符\0结尾的字符数组）
            int ret = cr->parse(body.c_str(),body.c_str() + body.size(), &val, &err);
            if(ret == false) // 有错误
            {
                ELOG("Json unserialize failed: %s \n" , err.c_str());
                return false;
            }
            return true;
        }
    };

    class UUID // 生成全局唯一标识符（Universally Unique Identifier），用于标识消息、连接、请求等实体。
    {
        public:
            static std::string uuid() // 构造一个随机ID
            // static: 意味着不需要实例化对象（new UUID()）就可以直接调用，例如 UUID::uuid()
            {
                std::stringstream ss; // 构建返回的string流，效率比直接用 + 拼接字符串高。
                std::random_device rd;// 1.构造一个机器随机数对象（种子）
                std::mt19937 generator(rd());// 2.以 rd 生成随机数生成器
                std::uniform_int_distribution<int> distribution(0, 255); // 3.构造限定数据范围的对象(2位十六进制)，0x00 - 0xFF 的范围
                for(int i = 0; i < 8; i++) // 4. ⽣成8个 2位的16进制随机数，按需求排序
                {
                    if(i == 4 || i == 6) ss << "-"; // 在第4个和第6个字节之前插入连字符 -，用于格式化输出。
                    // setw: 设置宽度 setfill: 不满2位时用0填充 std::hex: 后续输出的整数为16进制显示
                    ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
                }
                ss << "-"; // XXXXXXXX-XXXX-XXXX-
                // 5.定义⼀个8字节序号（一字节8比特位），逐字节组织成为16进制数字字符的字符串 // 0000 0001
                static std::atomic<size_t> seq(1); // 定义一个线程安全的静态计数器seq，seq 变量在内存中只有一份，它会随着每次调用而递增
                // std::atomic: 原子类型，保证在多线程环境下，多个线程同时调用 uuid() 时，计数器的增加是线程安全的
                size_t cur = seq.fetch_add(1); // 获取当前值并递增seq
                for (int i = 7; i >= 0; i--) { // 将一个size_t类型的变量（cur）转化为8个16进制字符
                    if (i == 5) ss << "-";// 从最高位到最低位的8个16进制
                    //cur >> (i*8): 将目标字节移动到最低位（最右边），依次打出从高位到低位的数字（0x12345678）
                    //& 0xFF：0000 0000 & 1111 1111 只保留最低的8位（1个字节），把高位全部清零。
                    //将长整数 cur 拆解为8个独立的字节，并依次打印为16进制。
                    ss << std::setw(2) << std::setfill('0') << std::hex << ((cur >> (i*8)) & 0xFF);
                }// SSSS-SSSSSSSSSSSS
                return ss.str();
            }
    };
}
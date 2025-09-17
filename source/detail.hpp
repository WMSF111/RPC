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

namespace bitrbc
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

    class UUID
    {
        public:
            static std::string uuid()
            {
                std::stringstream ss; // 构建返回的string
                std::random_device rd;// 1.构造一个机器随机数对象（种子）
                std::mt19937 generator(rd());// 2.以硬件随机数为种子构造伪随机对象(将rd作为)
                std::uniform_int_distribution<int> distribution(0, 255); // 3.构造限定数据范围的对象
                for(int i = 0; i < 8; i++) // 4. ⽣成8个2位的16进制随机数，按需求排序
                {
                    if(i == 4 || i == 6) ss << "-";
                    // setw: 设置位数 setfill: 不满2位时用0填充 std::hex: 16进制显示
                    ss << std::setw(2) << std::setfill('0') << std::hex << distribution(generator);
                }
                ss << "-";
                // 5.定义⼀个8字节序号（一字节8比特位），逐字节组织成为16进制数字字符的字符串 // 0000 0001
                static std::atomic<size_t> seq(1); // 定义一个线程安全的静态计数器seq，它会随着每次调用而递增
                size_t cur = seq.fetch_add(1); // 获取当前值并递增seq
                for (int i = 7; i >= 0; i--) { // 将一个size_t类型的变量（cur）转化为8个16进制字符
                    if (i == 5) ss << "-";// 从最高位到最低位的8个16进制
                    //0000 0001 一字节8比特位
                    //0000 0000 & 1111 1111 右移7位
                    //00 输出为16字节2位
                    //0000-000000000001
                    ss << std::setw(2) << std::setfill('0') << std::hex << ((cur >> (i*8)) & 0xFF);
                }
                return ss.str();
            }
    };
}
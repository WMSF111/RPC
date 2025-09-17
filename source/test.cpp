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

int main()
{
    UUID id;
    for(int i = 0; i < 10; i++)
        std::cout << id.uuid() << std::endl;
}


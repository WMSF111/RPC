#include<iostream>
#include<string>
#include <sstream>
#include<jsoncpp/json/json.h>

/*输入：JsonValue类，要转化的string类；
目的：将JsonValue其转化为string类
返回：转化成功与否*/
bool serialize(Json::Value &val, std::string &body) // 序列化函数
{
    Json::StreamWriterBuilder swb;
    swb["emitUTF8"] = true;
    std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter()); // 创建新的StreamWriter类
    std::stringstream ss; // 转换成字节流格式，方便传给write。
    int ret = sw->write(val, &ss); //读取JSONValue类val给ss
    if(ret != 0)
    {
        std::cout << "Json serialize failed."  << std::endl;
        return false;
    }
    body = ss.str(); // 转换为string格式，方便调用输出
    return true;
}

/*输入：JsonValue类，要转化的string类；
目的：将string类转化为JsonValue类
返回：转化成功与否*/
bool unserialize(Json::Value &val, std::string &body)
{
    Json::CharReaderBuilder crb;
    std::string err;
    std::unique_ptr<Json::CharReader> cr(crb.newCharReader());
    // c_str：转换成 C 风格的字符串（即以null字符\0结尾的字符数组）
    int ret = cr->parse(body.c_str(),body.c_str() + body.size(), &val, &err);
    if(ret == false) // 有错误
    {
        std::cout << "Json unserialize failed: " << err << std::endl;
        return false;
    }
    return true;
}

int main()
{
    Json::Value pe;
    const char* name = "小武";
    const char* sex = "男";
    int age = 23;
    int fix[2] = {85, 180};
    Json::Value fav;
    fav["书籍"] = "明朝那些事";
    fav["游戏"] = "逆战";
    pe["姓名"] = name;
    pe["性别"] = sex;
    pe["年龄"] = age;
    pe["身材"].append(fix[0]);
    pe["身材"].append(fix[1]);
    pe["爱好"] = fav;
    std::string ss;
    bool ret = serialize(pe, ss);
    std::cout << ss << std::endl;
    std::cout << std::endl;

    std::string jsonstr = R"({"姓名":"小武","年龄":23,
                    "爱好":{"书籍" : "明朝那些事","游戏":"逆战"},
                    "身材":[85,180]})";
    // std::string jsonstr = ss;
    Json::Value unserVal;
    ret = unserialize(unserVal, jsonstr);
    std::cout << "姓名：" << unserVal["姓名"].asString() << std::endl;
    std::cout << "年龄：" << unserVal["年龄"].asInt() << std::endl;
    std::cout << "爱好书籍：" << unserVal["爱好"]["书籍"].asString() << std::endl;
    std::cout << "爱好游戏：" << unserVal["爱好"]["游戏"].asString() << std::endl;
    int size = unserVal["身材"].size();
    for (int i = 0 ; i < size; i++)
        std::cout << "身材：" << unserVal["身材"][i].asString() << std::endl;

    return 0;
}
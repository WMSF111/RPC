#include<iostream>
#include<future>
#include <thread>

int ADD(int a, int b)
{
    std::cout << "into add!\n";
    return a + b;
}

int main()
{
    // std::future<int> res = std::async(std::launch::async, ADD, 11, 22); //先运行ADD
    std::future<int> res = std::async(std::launch::deferred, ADD, 11, 22); //get的时候再运行
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "-------------------------------------------------";
    std::cout << "获取结果是：" << res.get() << std::endl;
    return 0;
}
#include<iostream>
#include<future>
#include<thread>

int ADD(int a, int b)
{
    std::cout << "into ADD\n";
    return a + b;
}

int main()
{
    std::promise<int> pro; // 1.实例化promise模板
    std::future<int> res = pro.get_future(); // 获取future对象
    std::thread thr([&pro](){ // 任意位置给pro设置结果数据
        int sum = ADD(11,22);
        pro.set_value(sum);
    });
    std::cout << res.get() << std::endl;
    thr.join();
    return 0;
}
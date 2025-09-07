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
    auto task = std::make_shared<std::packaged_task<int(int,int)>>(ADD); // 为封装任务创建智能指针方便后面线程调用
    std::future<int> res = task->get_future(); // 获取任务包关联的future对象
    std::thread thr([task](){
        (*task)(11, 22);
    });
    std::cout << res.get() << std::endl; //输出任务结果

    // 这是同步任务，在main中调用
    // std::packaged_task<int(int,int)> task(ADD); // 封装任务
    // std::future<int> res = task.get_future(); //获取任务包关联的future对象
    // task(11,22); //执行任务
    // std::cout << res.get() << std::endl; //输出任务结果
    thr.join(); // 线程等待，如果没有main会不管子线程退出
    return 0;
}
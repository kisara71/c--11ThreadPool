#include "ThreadPool.h"
#include <iostream>
#include <chrono>
#include <mutex>
void task(int a, int b)
{
    
    {
        std::lock_guard<std::mutex> lock(ThreadPool::m_outputMutex);
        std::cout << "Task " << a << " + " << b << " = " << a + b << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
}
int main()
{
    system("chcp 65001");
   {
     ThreadPool pool;
    for (int i = 0; i < 10; i++)
    {
        pool.addTask(std::bind(task, i, i+1));
    }
    getchar();
    {
        std::lock_guard<std::mutex> lock(ThreadPool::m_outputMutex);
        std::cout<<"Total workers: "<<pool.getTotalWorkers()<<std::endl;
        std::cout<<"Idle workers: "<<pool.getIdleWorkers()<<std::endl;
        std::cout<<"Min workers: "<<pool.getMinWorkers()<<std::endl;
    }
    for (int i = 20; i < 40; i++)
    {
        pool.addTask(std::bind(task, i, i+1));
    }
    getchar();
    }
    system("pause");
    return 0;
}
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>
#include <vector>
#include <iostream>
#include <future>
#include <memory>
class ThreadPool
{

public:
    ThreadPool(uint16_t min = 4, uint16_t max = std::thread::hardware_concurrency());
    ~ThreadPool();

public:
    //void addTask(std::function<void(void)> task);
    template<typename F, typename... Args>
    auto addTask(F&& f, Args&&... args)->std::future<typename std::invoke_result_t<F, Args...>>
    {
        using returnType = typename std::invoke_result_t<F,Args...>;
        auto taskPtr = std::make_shared<std::packaged_task<returnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<returnType> res = taskPtr->get_future();
        {
            std::unique_lock<std::mutex>lock(m_tasksMutex);
            m_tasks.emplace([taskPtr](){
                (*taskPtr)();
            });
            m_condition.notify_one();
        }
        return res;
    }
    static std::mutex m_outputMutex;
    uint16_t getTotalWorkers() const { return m_totalWorkers; }
    uint16_t getIdleWorkers() const { return m_idleWorkers; }
    uint16_t getMinWorkers() const { return m_minWorkers; }

private:
    void worker();
    void monitor();

private:
    std::queue<std::function<void(void)>> m_tasks;
    std::map<std::thread::id, std::thread> m_workers;
    std::vector<std::thread::id> m_destroyWorkers;

    std::mutex m_tasksMutex;
    std::condition_variable m_condition;

    std::atomic<bool> m_stop;
    std::atomic<uint16_t> m_exitWorkers;
    std::atomic<uint16_t> m_idleWorkers;
    std::atomic<uint16_t> m_totalWorkers;
    std::atomic<uint16_t> m_maxWorkers;
    std::atomic<uint16_t> m_minWorkers;

    std::thread m_monitor;
    
};




std::mutex ThreadPool::m_outputMutex;
ThreadPool::ThreadPool(uint16_t min, uint16_t max) :
 m_stop(false),
m_idleWorkers(min),
m_totalWorkers(min),
m_maxWorkers(max),
m_minWorkers(min),
m_exitWorkers(0)
{
    for (uint16_t i; i < min; i++)
    {
        std::thread worker(&ThreadPool::worker, this);
        m_workers[worker.get_id()] = std::move(worker);
    }
    m_monitor = std::thread(&ThreadPool::monitor, this);
}

// void ThreadPool::addTask(std::function<void(void)> task)
// {
//     std::lock_guard<std::mutex> lock(m_tasksMutex);
//     m_tasks.push(task);
//     m_condition.notify_one();
// }
void ThreadPool::worker()
{
    while (!m_stop)
    {
        std::function<void(void)> task;
        {
            std::unique_lock<std::mutex> lock(m_tasksMutex);
            m_condition.wait(lock, [this]()
                             { return m_stop || !m_tasks.empty() || m_exitWorkers > 0; });
            if (m_stop)
                return;
            if (m_exitWorkers > 0)
            {
                m_exitWorkers--;
                m_totalWorkers--;
                m_idleWorkers--;
                {
                    std::lock_guard<std::mutex> lock(m_outputMutex);
                    std::cout << "Worker " << std::this_thread::get_id() << " is exiting\n";
                }
                m_destroyWorkers.push_back(std::this_thread::get_id());
                return;
            }
            if (!m_tasks.empty())
            {
                task = m_tasks.front();
                m_tasks.pop();
            }
        }
        m_idleWorkers--;
        task();
        m_idleWorkers++;
        {
            std::lock_guard<std::mutex> lock(m_outputMutex);
            std::cout << "Worker " << std::this_thread::get_id() << " is idle\n";
        }
    }
}
void ThreadPool::monitor()
{
    while (!m_stop)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (m_stop)
            return;
        if (m_idleWorkers > m_totalWorkers / 2 && m_totalWorkers > m_minWorkers)
        {
            m_exitWorkers.store(m_totalWorkers / 3);
            m_condition.notify_all();
            std::unique_lock<std::mutex> lock(m_tasksMutex);
            for (auto &worker : m_destroyWorkers)
            {
                if (m_workers[worker].joinable())
                {
                    m_workers[worker].join();
                }
                m_workers.erase(worker);
            }
        }
        else if (m_idleWorkers == 0 && m_totalWorkers < m_maxWorkers)
        {
            std::unique_lock<std::mutex> lock(m_tasksMutex);
            std::thread worker(&ThreadPool::worker, this);
            m_workers[worker.get_id()] = std::move(worker);
            m_totalWorkers++;
            m_idleWorkers++;
            {
                std::lock_guard<std::mutex> lock(m_outputMutex);
                std::cout << "Worker " << worker.get_id() << " is added\n";
            }
        }
    }
}
ThreadPool::~ThreadPool()
{
    m_stop = true;
    m_condition.notify_all();
    m_monitor.join();
    {
        std::lock_guard<std::mutex> lock(m_outputMutex);
        std::cout << "Monitor is joining\n";
    }
    for (auto &worker : m_workers)
    {
        if (worker.second.joinable())
        {
            {
                std::lock_guard<std::mutex> lock(m_outputMutex);
                std::cout << "Worker " << worker.first << " is joining\n";
            }
            worker.second.join();
        }
    }
}

#endif
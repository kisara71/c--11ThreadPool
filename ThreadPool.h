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
class ThreadPool {

public:
    ThreadPool(uint16_t min= 4, uint16_t max=std::thread::hardware_concurrency());
    ~ThreadPool();

public:
    void addTask(std::function<void(void)> task);
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

    std::thread m_monitor;;
};
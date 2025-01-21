#include "ThreadPool.h"
#include <iostream>
std::mutex ThreadPool::m_outputMutex;
ThreadPool::ThreadPool(uint16_t min, uint16_t max):
m_stop(false),
m_idleWorkers(min),
m_totalWorkers(min),
m_maxWorkers(max),
m_minWorkers(min),
m_exitWorkers(0)
{
    for (uint16_t i; i<min; i++){
        std::thread worker(&ThreadPool::worker, this);
        m_workers[worker.get_id()] = std::move(worker);
    }
    m_monitor = std::thread(&ThreadPool::monitor, this);
}

void ThreadPool::addTask(std::function<void(void)> task){
    std::lock_guard<std::mutex> lock(m_tasksMutex);
    m_tasks.push(task);
    m_condition.notify_one();
}
void ThreadPool::worker()
{
    while(!m_stop){
        std::function<void(void)> task;
        {
            std::unique_lock<std::mutex> lock(m_tasksMutex);
            m_condition.wait(lock, [this](){return m_stop || !m_tasks.empty() || m_exitWorkers > 0;});
            if(m_stop) return;
            if(m_exitWorkers > 0){
                m_exitWorkers--;
                m_totalWorkers--;
                m_idleWorkers--;
                {
                    std::lock_guard<std::mutex> lock(m_outputMutex);
                    std::cout<<"Worker "<<std::this_thread::get_id()<<" is exiting\n";
                }
                m_destroyWorkers.push_back(std::this_thread::get_id());
                return;
            }
            if(!m_tasks.empty()){
                task = m_tasks.front();
                m_tasks.pop();
            }
        }
        m_idleWorkers--;
        task();
        m_idleWorkers++;
        {
            std::lock_guard<std::mutex> lock(m_outputMutex);
            std::cout<<"Worker "<<std::this_thread::get_id()<<" is idle\n";
        }
    }
}
void ThreadPool::monitor(){
    while(!m_stop){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(m_stop) return;
        if(m_idleWorkers > m_totalWorkers/2 && m_totalWorkers > m_minWorkers){
            m_exitWorkers.store(m_totalWorkers / 3);
            m_condition.notify_all();
            std::unique_lock<std::mutex> lock(m_tasksMutex);
            for(auto& worker: m_destroyWorkers){
                if(m_workers[worker].joinable()){
                    m_workers[worker].join();
                }
                m_workers.erase(worker);
            }
        }else if (m_idleWorkers == 0 && m_totalWorkers<m_maxWorkers){
            std::unique_lock<std::mutex> lock(m_tasksMutex);
            std::thread worker(&ThreadPool::worker, this);
            m_workers[worker.get_id()] = std::move(worker);
            m_totalWorkers++;
            m_idleWorkers++;
            {
                std::lock_guard<std::mutex> lock(m_outputMutex);
                std::cout<<"Worker "<<worker.get_id()<<" is added\n";
            }
        }
        
    }
}
ThreadPool::~ThreadPool(){
    m_stop = true;
    m_condition.notify_all();
    m_monitor.join();
    {
        std::lock_guard<std::mutex> lock(m_outputMutex);
        std::cout<<"Monitor is joining\n";
    }
    for(auto& worker: m_workers){
        if (worker.second.joinable()){
            {
                std::lock_guard<std::mutex> lock(m_outputMutex);
                std::cout<<"Worker "<<worker.first<<" is joining\n";
            }
            worker.second.join();
        }
    }
}
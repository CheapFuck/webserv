#include "thread-pool.hpp"

// Constructor - starts the thread pool
ThreadPool::ThreadPool(size_t numThreads) : _activeTaskCount(0), _stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        _workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(this->_queueMutex);
                    
                    // Wait for a task or stop signal
                    this->_condition.wait(lock, [this] { 
                        return this->_stop || !this->_tasks.empty(); 
                    });
                    
                    // Exit if stopping and no tasks
                    if (this->_stop && this->_tasks.empty()) {
                        return;
                    }
                    
                    // Get the next task
                    task = std::move(this->_tasks.front());
                    this->_tasks.pop();
                }
                
                // Execute the task
                _activeTaskCount++;
                task();
                _activeTaskCount--;
            }
        });
    }
}

// Destructor - stops the thread pool
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _stop = true;
    }
    
    _condition.notify_all();
    
    // Join all threads
    for (std::thread &worker : _workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

// Get the number of active tasks
size_t ThreadPool::activeTaskCount() const {
    return _activeTaskCount;
}
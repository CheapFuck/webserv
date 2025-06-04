#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Add a task to the thread pool and get a future for the result
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // Get the number of active tasks
    size_t activeTaskCount() const;

private:
    // Worker threads
    std::vector<std::thread> _workers;
    
    // Task queue
    std::queue<std::function<void()>> _tasks;
    
    // Synchronization
    mutable std::mutex _queueMutex;
    std::condition_variable _condition;
    std::atomic<size_t> _activeTaskCount;
    bool _stop;
};

// Template method implementation must be in header
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    // using return_type = typename std::result_of<F(Args...)>::type;
    using return_type = typename std::invoke_result<F, Args...>::type;

    
    // Create a shared pointer to the packaged task
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    // Get the future result
    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        
        // Don't allow enqueueing after stopping the pool
        if (_stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        
        // Add the task to the queue
        _tasks.emplace([task]() { (*task)(); });
    }
    
    // Notify one waiting thread
    _condition.notify_one();
    
    return res;
}
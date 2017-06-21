#ifndef thread_pool_hpp
#define thread_pool_hpp

#include <thread>
#include <mutex>
#include <queue>
#include <functional>

using std::queue;
using std::mutex;
using std::bind;

template <class T> struct Data {
    bool ready;
    T data;
    Data() : ready(false) {}
};

using functionType = std::function<void()>;

class Worker {
private:
    bool enabled;
    queue<functionType> queueOfFunctions;
    std::mutex m;
    std::thread thread;
    
    std::condition_variable cv;
public:
    Worker(): enabled(true), queueOfFunctions(), thread(&Worker::threadFucntion, this) {}
    
    ~Worker() {
        enabled = false;
        cv.notify_one();
        thread.join();
    }
    
    void appendFunction(functionType function) {
        std::lock_guard<mutex> locker(m);
        queueOfFunctions.push(function);
        cv.notify_one();
    }
    
    size_t count() {
        std::unique_lock<std::mutex> locker(m);
        return queueOfFunctions.size();
    }
    
    bool isEmpty() {
        std::unique_lock<std::mutex> locker(m);
        return queueOfFunctions.empty();
    }
    
    void threadFucntion() {
        while (enabled) {
            std::unique_lock<mutex> locker(m);
            
            // Wait for notification
            // Thread wakes up if queue is not empty and thread is off
            cv.wait(locker, [&](){ return !queueOfFunctions.empty() || !enabled; });
            while(!queueOfFunctions.empty()) {
                functionType function = queueOfFunctions.front();
                // Unclock mutex before calling functor
                locker.unlock();
                function();
                // Lock bebore (!) fqueue.empty()
                locker.lock();
                queueOfFunctions.pop();
            }
        }
    }
};

class ThreadPool {
public:
    
    ThreadPool(size_t numberOfThreads = 1) {
        if (numberOfThreads == 0) {
            numberOfThreads = 1;
        }
        
        for (size_t i(0); i < numberOfThreads; i++) {
            workers.push_back(workerPtr(new Worker));
        }
    }
    // Void function with any parameters
    template<typename FuncType, typename... Types>
    void runAsync(FuncType _f, Types... params) {
        auto funcToCall = bind(_f, params...);
        getFreeWorker()->appendFunction(funcToCall);
    }
    
private:
    using workerPtr = std::shared_ptr<Worker>;
    std::vector<workerPtr> workers;
    
    workerPtr getFreeWorker() {
        workerPtr pWorker;
        size_t min = UINT32_MAX;
        
        for (auto &it : workers) {
            if (it->isEmpty()) {
                return it;
            } else if (min > it->count()) {
                min = it->count();
                pWorker = it;
            }
        }
        return pWorker;
    }
};

#endif /* thread_pool_hpp */
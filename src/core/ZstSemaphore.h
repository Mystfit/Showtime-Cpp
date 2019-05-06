//Thanks to https://stackoverflow.com/questions/20324327/producer-consumer-lost-wake-up-issue

#pragma once

#include <mutex>
#include <utility>
#include <condition_variable>

class ZstSemaphore
{
public: 
    ZstSemaphore(int count_ = 0) : count(count_) { }

    void notify()
    {
        std::unique_lock<std::mutex> lck(mtx);
        ++count;
        cv.notify_one();
    }

    void wait() { return wait([]{}); }  // no-op action

    template <typename F>
    auto wait(F&& func = []{}) -> decltype(std::declval<F>()())
    {
        std::unique_lock<std::mutex> lck(mtx);

        while(count == 0){
            cv.wait(lck);
        }
        count--;

        return func();
    }
    
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

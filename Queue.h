#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class Queue
{
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<T> queue_;
public:
    T pop()
    {
        std::unique_lock<std::mutex> lock{mutex_};

        cond_.wait(lock, [this](){return !queue_.empty();});

        auto value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    template <typename R, typename P>
    T *pop(T &value, std::chrono::duration<R, P> timeout)
    {
        std::unique_lock<std::mutex> lock{mutex_};

        const auto status =
            cond_.wait_for(lock, timeout, [this](){return !queue_.empty();});

        if(!status) return nullptr;

        value = std::move(queue_.front());
        queue_.pop();
        return &value;
    }

    void push(T value)
    {
        std::unique_lock<std::mutex> lock{mutex_};

        queue_.push(std::move(value));
        cond_.notify_one();
    }
};

#pragma once

template <typename T> class MutexQueue {
  public:
    using value_type = T;

    bool try_push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(item);
        return true;
    }

    bool try_pop(T& out) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!queue_.empty()) {
            out = queue_.front();
            queue_.pop();
            return true;
        }

        return false;
    }

  private:
    std::queue<T> queue_;
    std::mutex mutex_;
};
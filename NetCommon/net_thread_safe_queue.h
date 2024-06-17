#pragma once
#include <deque>
#include <mutex>

#include "net_common.h"

namespace olc {
namespace net {

template <typename T> class threadSafeQueue {
public:
  threadSafeQueue() = default;
  threadSafeQueue(const threadSafeQueue<T> &) = default;
  virtual ~threadSafeQueue() { clear(); }

public:
  const T &front() {
    std::scoped_lock lock(muxQueue);
    return deqQueue.front();
  }

  const T &back() {
    std::scoped_lock lock(muxQueue);
    return deqQueue.back();
  }

  // Adds item to the back of the queue
  void push_back(const T &item) {
    std::scoped_lock lock(muxQueue);
    deqQueue.emplace_back(std::move(item));
  }

  // Adds item to the front of the queue
  void push_front(const T &item) {
    std::scoped_lock lock(muxQueue);
    deqQueue.emplace_front(std::move(item));
  }

  // Returns true if the queue is empty
  bool empty() {
    std::scoped_lock lock(muxQueue);
    return deqQueue.empty();
  }

  // Returns the size of the queue
  size_t count() {
    std::scoped_lock lock(muxQueue);
    return deqQueue.size();
  }

  void clear() {
    std::scoped_lock lock(muxQueue);
    deqQueue.clear();
  }
  // Removes and returns the item from the front of the queue.
  T pop_front() {
    std::scoped_lock lock(muxQueue);
    auto t = std::move(deqQueue.front());
    deqQueue.pop_front();
    return t;
  }

  T pop_back() {
    std::scoped_lock lock(muxQueue);
    auto t = std::move(deqQueue.back());
    deqQueue.pop_back();
    return t;
  }

protected:
  std::mutex muxQueue;
  std::deque<T> deqQueue;
};

threadSafeQueue::threadSafeQueue() {}

threadSafeQueue::~threadSafeQueue() {}
} // namespace net
} // namespace olc

// thread-pool.hpp
// Vibe coded with Gemini.

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
 public:
  // Constructor for the ThreadPool.
  // It initializes a specified number of worker threads.
  ThreadPool(size_t threads) : stop(false) {
    // Loop to create 'threads' number of worker threads.
    for (size_t i = 0; i < threads; ++i) {
      // emplace_back constructs a new thread in place.
      // The thread's main function is a lambda that captures `this`.
      workers.emplace_back([this] {
        // The worker thread's infinite loop. It will run until the pool is
        // stopped.
        while (true) {
          std::function<void()> task;  // Placeholder for a task to be executed.
          {
            // A unique_lock is used to acquire a lock on the mutex.
            // It automatically releases the lock when it goes out of scope
            // (RAII).
            std::unique_lock<std::mutex> lock(this->queue_mutex);

            // The condition variable's `wait` function blocks the current
            // thread. It waits until the specified predicate (the lambda) is
            // true. The predicate returns true if either the pool is stopping
            // or the task queue is not empty. This prevents a "spurious
            // wakeup," where the thread wakes up for no reason.
            this->condition.wait(
                lock, [this] { return this->stop || !this->tasks.empty(); });

            // Check if the thread should terminate.
            // This happens if the pool is stopping and there are no more tasks.
            if (this->stop && this->tasks.empty()) {
              return;  // Exit the worker thread's main loop.
            }

            // Move the task from the front of the queue into the local `task`
            // variable. std::move is used for efficiency, as we're taking
            // ownership of the task.
            task = std::move(this->tasks.front());
            // Remove the task from the queue.
            this->tasks.pop();
          }  // The unique_lock goes out of scope here, automatically releasing
             // the mutex. This allows other threads to access the queue.

          // Execute the retrieved task.
          task();
        }
      });
    }
  }

  // Template function to add a new task to the queue.
  // F is the function type, and Args are the argument types.
  template <class F, class... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type> {
    // Use std::result_of to determine the return type of the function call.
    using return_type = typename std::result_of<F(Args...)>::type;

    // Create a packaged_task. This wraps the callable object and allows
    // its result to be retrieved via a future.
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        // std::bind is used to "bind" the function 'f' with its arguments
        // 'args'. std::forward is used for perfect forwarding, preserving the
        // value category of arguments.
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    // Get a future associated with the packaged_task.
    // The future will eventually hold the result of the task.
    std::future<return_type> res = task->get_future();
    {
      // Acquire a unique_lock to protect the task queue.
      std::unique_lock<std::mutex> lock(queue_mutex);

      // Check if the pool is in the process of shutting down.
      if (stop) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }

      // Add the packaged_task to the queue.
      // The lambda `[task]() { (*task)(); }` captures the shared_ptr to the
      // packaged_task. This ensures the task object is not destroyed until a
      // worker thread can execute it.
      tasks.emplace([task]() { (*task)(); });
    }  // Lock is released here.

    // Notify one of the waiting worker threads that a new task is available.
    condition.notify_one();

    // Return the future, allowing the caller to retrieve the task's result
    // later.
    return res;
  }

  // Destructor for the ThreadPool.
  // It stops all worker threads and joins them.
  ~ThreadPool() {
    {
      // Acquire a lock to set the stop flag.
      std::unique_lock<std::mutex> lock(queue_mutex);
      // Set the stop flag to true, signaling the threads to terminate.
      stop = true;
    }  // The lock is released here.

    // Notify all waiting threads to wake up.
    // They will see the `stop` flag is true and exit their loops.
    condition.notify_all();

    // Loop through all worker threads and wait for each one to finish.
    // `join` blocks the main thread until the worker thread has completed its
    // execution.
    for (std::thread& worker : workers) {
      worker.join();
    }
  }

 private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  bool stop;
};
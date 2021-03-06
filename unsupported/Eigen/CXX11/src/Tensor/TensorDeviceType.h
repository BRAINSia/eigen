// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2014 Benoit Steiner <benoit.steiner.goog@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef EIGEN_CXX11_TENSOR_TENSOR_DEVICE_TYPE_H
#define EIGEN_CXX11_TENSOR_TENSOR_DEVICE_TYPE_H


namespace Eigen {

// Default device for the machine (typically a single cpu core)
struct DefaultDevice {
  EIGEN_STRONG_INLINE void* allocate(size_t num_bytes) const {
    return internal::aligned_malloc(num_bytes);
  }
  EIGEN_STRONG_INLINE void deallocate(void* buffer) const {
    internal::aligned_free(buffer);
  }
  EIGEN_STRONG_INLINE void memcpy(void* dst, const void* src, size_t n) const {
    ::memcpy(dst, src, n);
  }
  EIGEN_STRONG_INLINE void memset(void* buffer, int c, size_t n) const {
    ::memset(buffer, c, n);
  }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE size_t numThreads() const {
#ifndef __CUDA_ARCH__
    // Running on the host CPU
    return 1;
#else
    // Running on a CUDA device
    return 32;
#endif
  }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE int majorDeviceVersion() const {
#ifndef __CUDA_ARCH__
    // Running single threaded on the host CPU
    // Should return an enum that encodes the ISA supported by the CPU
    return 1;
#else
    // Running on a CUDA device
    return __CUDA_ARCH__ / 100;
#endif
  }
};


// Multiple cpu cores
// We should really use a thread pool here but first we need to find a portable thread pool library.
#ifdef EIGEN_USE_THREADS

// The implementation of the ThreadPool type ensures that the Schedule method
// runs the functions it is provided in FIFO order when the scheduling is done
// by a single thread.
class ThreadPool {
 public:
  // Construct a pool that contains "num_threads" threads.
  explicit ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; i++) {
      threads_.push_back(new std::thread([this]() { WorkerLoop(); }));
    }
  }

  // Wait until all scheduled work has finished and then destroy the
  // set of threads.
  ~ThreadPool()
  {
    {
      // Wait for all work to get done.
      std::unique_lock<std::mutex> l(mu_);
      empty_.wait(l, [this]() { return pending_.empty(); });
      exiting_ = true;

      // Wakeup all waiters.
      for (auto w : waiters_) {
        w->ready = true;
        w->work = nullptr;
        w->cv.notify_one();
      }
    }

    // Wait for threads to finish.
    for (auto t : threads_) {
      t->join();
      delete t;
    }
  }

  // Schedule fn() for execution in the pool of threads. The functions are
  // executed in the order in which they are scheduled.
  void Schedule(std::function<void()> fn) {
    std::unique_lock<std::mutex> l(mu_);
    if (waiters_.empty()) {
      pending_.push_back(fn);
    } else {
      Waiter* w = waiters_.back();
      waiters_.pop_back();
      w->ready = true;
      w->work = fn;
      w->cv.notify_one();
    }
  }

 protected:
  void WorkerLoop() {
    std::unique_lock<std::mutex> l(mu_);
    Waiter w;
    while (!exiting_) {
      std::function<void()> fn;
      if (pending_.empty()) {
        // Wait for work to be assigned to me
        w.ready = false;
        waiters_.push_back(&w);
        w.cv.wait(l, [&w]() { return w.ready; });
        fn = w.work;
        w.work = nullptr;
      } else {
        // Pick up pending work
        fn = pending_.front();
        pending_.pop_front();
        if (pending_.empty()) {
          empty_.notify_all();
        }
      }
      if (fn) {
        mu_.unlock();
        fn();
        mu_.lock();
      }
    }
  }

 private:
  struct Waiter {
    std::condition_variable cv;
    std::function<void()> work;
    bool ready;
  };

  std::mutex mu_;
  std::vector<std::thread*> threads_;               // All threads
  std::vector<Waiter*> waiters_;                    // Stack of waiting threads.
  std::deque<std::function<void()>> pending_;       // Queue of pending work
  std::condition_variable empty_;                   // Signaled on pending_.empty()
  bool exiting_ = false;
};


// Notification is an object that allows a user to to wait for another
// thread to signal a notification that an event has occurred.
//
// Multiple threads can wait on the same Notification object.
// but only one caller must call Notify() on the object.
class Notification {
 public:
  Notification() : notified_(false) {}
  ~Notification() {}

  void Notify() {
    std::unique_lock<std::mutex> l(mu_);
    eigen_assert(!notified_);
    notified_ = true;
    cv_.notify_all();
  }

  void WaitForNotification() {
    std::unique_lock<std::mutex> l(mu_);
    cv_.wait(l, [this]() { return notified_; } );
  }

 private:
  std::mutex mu_;
  std::condition_variable cv_;
  bool notified_;
};

// Runs an arbitrary function and then calls Notify() on the passed in
// Notification.
template <typename Function, typename... Args> struct FunctionWrapper
{
  static void run(Notification* n, Function f, Args... args) {
    f(args...);
    n->Notify();
  }
};

static EIGEN_STRONG_INLINE void wait_until_ready(Notification* n) {
  if (n) {
    n->WaitForNotification();
  }
}


// Build a thread pool device on top the an existing pool of threads.
struct ThreadPoolDevice {
  ThreadPoolDevice(ThreadPool* pool, size_t num_cores) : pool_(pool), num_threads_(num_cores) { }

  EIGEN_STRONG_INLINE void* allocate(size_t num_bytes) const {
    return internal::aligned_malloc(num_bytes);
  }

  EIGEN_STRONG_INLINE void deallocate(void* buffer) const {
    internal::aligned_free(buffer);
  }

  EIGEN_STRONG_INLINE void memcpy(void* dst, const void* src, size_t n) const {
    ::memcpy(dst, src, n);
  }

  EIGEN_STRONG_INLINE void memset(void* buffer, int c, size_t n) const {
    ::memset(buffer, c, n);
  }

  EIGEN_STRONG_INLINE size_t numThreads() const {
    return num_threads_;
  }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE int majorDeviceVersion() const {
    // Should return an enum that encodes the ISA supported by the CPU
    return 1;
  }

  template <class Function, class... Args>
  EIGEN_STRONG_INLINE Notification* enqueue(Function&& f, Args&&... args) const {
    Notification* n = new Notification();
    std::function<void()> func =
      std::bind(&FunctionWrapper<Function, Args...>::run, n, f, args...);
    pool_->Schedule(func);
    return n;
  }
  template <class Function, class... Args>
  EIGEN_STRONG_INLINE void enqueueNoNotification(Function&& f, Args&&... args) const {
    std::function<void()> func = std::bind(f, args...);
    pool_->Schedule(func);
  }

 private:
  ThreadPool* pool_;
  size_t num_threads_;
};

#endif


// GPU offloading
#ifdef EIGEN_USE_GPU
static cudaDeviceProp m_deviceProperties;
static bool m_devicePropInitialized = false;

static void initializeDeviceProp() {
  if (!m_devicePropInitialized) {
    assert(cudaGetDeviceProperties(&m_deviceProperties, 0) == cudaSuccess);
    m_devicePropInitialized = true;
  }
}

static inline int getNumCudaMultiProcessors() {
  initializeDeviceProp();
  return m_deviceProperties.multiProcessorCount;
}
static inline int maxCudaThreadsPerBlock() {
  initializeDeviceProp();
  return m_deviceProperties.maxThreadsPerBlock;
}
static inline int maxCudaThreadsPerMultiProcessor() {
  initializeDeviceProp();
  return m_deviceProperties.maxThreadsPerMultiProcessor;
}
static inline int sharedMemPerBlock() {
  initializeDeviceProp();
  return m_deviceProperties.sharedMemPerBlock;
}

static inline void setCudaSharedMemConfig(cudaSharedMemConfig config) {
  cudaError_t status = cudaDeviceSetSharedMemConfig(config);
  assert(status == cudaSuccess);
}

struct GpuDevice {
  // The cudastream is not owned: the caller is responsible for its initialization and eventual destruction.
  GpuDevice(const cudaStream_t* stream) : stream_(stream) { eigen_assert(stream); }

  EIGEN_STRONG_INLINE const cudaStream_t& stream() const { return *stream_; }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void* allocate(size_t num_bytes) const {
#ifndef __CUDA_ARCH__
    void* result;
    assert(cudaMalloc(&result, num_bytes) == cudaSuccess);
    assert(result != NULL);
    return result;
#else
    assert(false && "The default device should be used instead to generate kernel code");
    return NULL;
#endif
  }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void deallocate(void* buffer) const {
#ifndef __CUDA_ARCH__
    assert(buffer != NULL);
    assert(cudaFree(buffer) == cudaSuccess);
#else
    assert(false && "The default device should be used instead to generate kernel code");
#endif
  }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void memcpy(void* dst, const void* src, size_t n) const {
#ifndef __CUDA_ARCH__
    assert(cudaMemcpyAsync(dst, src, n, cudaMemcpyDeviceToDevice, *stream_) == cudaSuccess);
#else
    assert(false && "The default device should be used instead to generate kernel code");
#endif
  }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void memset(void* buffer, int c, size_t n) const {
#ifndef __CUDA_ARCH__
    assert(cudaMemsetAsync(buffer, c, n, *stream_) == cudaSuccess);
#else
    assert(false && "The default device should be used instead to generate kernel code");
#endif
  }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE size_t numThreads() const {
    // FIXME
    return 32;
  }

 inline int majorDeviceVersion() const { return m_deviceProperties.major; }

  EIGEN_DEVICE_FUNC EIGEN_STRONG_INLINE void synchronize() const {
    cudaStreamSynchronize(*stream_);
  }

 private:
  // TODO: multigpu.
  const cudaStream_t* stream_;
};

#define LAUNCH_CUDA_KERNEL(kernel, gridsize, blocksize, sharedmem, device, ...)            \
  (kernel) <<< (gridsize), (blocksize), (sharedmem), (device).stream() >>> (__VA_ARGS__);  \
  assert(cudaGetLastError() == cudaSuccess);

#endif

}  // end namespace Eigen

#endif // EIGEN_CXX11_TENSOR_TENSOR_DEVICE_TYPE_H

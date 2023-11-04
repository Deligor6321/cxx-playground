// Copyright 2023 Deligor <deligor6321@gmail.com>

#include <benchmark/benchmark.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <semaphore>
#include <shared_mutex>
#include <thread>

namespace {

void bm_concurrent_condvar_shared_mutex(benchmark::State& state) {
  bool ready = false;
  bool finish = false;
  std::shared_mutex mutex;
  std::condition_variable_any cond_var;

  auto other_thread = std::thread([&] {
    while (true) {
      auto lock = std::unique_lock(mutex);
      cond_var.wait(lock, [&] { return finish || ready; });
      if (finish) {
        return;
      }
      ready = false;
      lock.unlock();
      cond_var.notify_one();
    }
  });

  for ([[maybe_unused]] auto _it : state) {
    auto lock = std::unique_lock(mutex);
    cond_var.wait(lock, [&] { return !ready; });
    ready = true;
    lock.unlock();
    cond_var.notify_one();
  }

  auto lock = std::unique_lock(mutex);
  cond_var.wait(lock, [&] { return !ready; });

  finish = true;
  lock.unlock();
  cond_var.notify_one();

  other_thread.join();
}
void bm_concurrent_condvar_mutex(benchmark::State& state) {
  bool ready = false;
  bool finish = false;
  std::mutex mutex;
  std::condition_variable cond_var;

  auto other_thread = std::thread([&] {
    while (true) {
      auto lock = std::unique_lock(mutex);
      cond_var.wait(lock, [&] { return finish || ready; });
      if (finish) {
        return;
      }
      ready = false;
      lock.unlock();
      cond_var.notify_one();
    }
  });

  for ([[maybe_unused]] auto _it : state) {
    auto lock = std::unique_lock(mutex);
    cond_var.wait(lock, [&] { return !ready; });
    ready = true;
    lock.unlock();
    cond_var.notify_one();
  }

  auto lock = std::unique_lock(mutex);
  cond_var.wait(lock, [&] { return !ready; });

  finish = true;
  lock.unlock();
  cond_var.notify_one();

  other_thread.join();
}

void bm_concurrent_semaphore(benchmark::State& state) {
  auto ready_signal = std::binary_semaphore{1};
  auto start_signal = std::binary_semaphore{0};
  bool finish = false;

  auto other_thread = std::thread([&] {
    while (true) {
      start_signal.acquire();
      if (finish) {
        return;
      }
      ready_signal.release();
    }
  });

  for ([[maybe_unused]] auto _it : state) {
    ready_signal.acquire();
    start_signal.release();
  }

  ready_signal.acquire();

  finish = true;
  start_signal.release();

  other_thread.join();
}

void bm_concurrent_atomic(benchmark::State& state) {
  auto ready_state = std::atomic<bool>(true);
  bool finish = false;

  auto other_thread = std::thread([&] {
    while (true) {
      ready_state.wait(true, std::memory_order::acquire);
      if (finish) {
        return;
      }
      ready_state.store(true, std::memory_order::release);
      ready_state.notify_one();
    }
  });

  for ([[maybe_unused]] auto _it : state) {
    ready_state.wait(false, std::memory_order::acquire);
    ready_state.store(false, std::memory_order::release);
    ready_state.notify_one();
  }

  ready_state.wait(false, std::memory_order::acquire);

  finish = true;
  ready_state.store(false, std::memory_order::release);
  ready_state.notify_one();

  other_thread.join();
}

}  // namespace

// NOLINTBEGIN
BENCHMARK(bm_concurrent_condvar_shared_mutex);
BENCHMARK(bm_concurrent_condvar_mutex);
BENCHMARK(bm_concurrent_semaphore);
BENCHMARK(bm_concurrent_atomic);
// NOLINTEND

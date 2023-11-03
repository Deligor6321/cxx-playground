#include <benchmark/benchmark.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <semaphore>
#include <thread>

static void bench_condvar_mutex(benchmark::State &state) {
  bool ready = false;
  bool finish = false;
  std::mutex mutex;
  std::condition_variable cond_var;

  auto other_thread = std::thread([&] {
    while (true) {
      auto lk = std::unique_lock(mutex);
      cond_var.wait(lk, [&] { return finish || ready; });
      if (finish) {
        return;
      }
      ready = false;
      lk.unlock();
      cond_var.notify_one();
    }
  });

  for (auto _ : state) {
    auto lk = std::unique_lock(mutex);
    cond_var.wait(lk, [&] { return !ready; });
    ready = true;
    lk.unlock();
    cond_var.notify_one();
  }

  auto lk = std::unique_lock(mutex);
  cond_var.wait(lk, [&] { return !ready; });

  finish = true;
  lk.unlock();
  cond_var.notify_one();

  other_thread.join();
}
BENCHMARK(bench_condvar_mutex);

static void bench_semaphore(benchmark::State &state) {
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

  for (auto _ : state) {
    ready_signal.acquire();
    start_signal.release();
  }

  ready_signal.acquire();

  finish = true;
  start_signal.release();

  other_thread.join();
}
BENCHMARK(bench_semaphore);

static void bench_atomic(benchmark::State &state) {
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

  for (auto _ : state) {
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
BENCHMARK(bench_atomic);

BENCHMARK_MAIN();
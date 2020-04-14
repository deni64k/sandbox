#include <cstdlib>
#include <ctime>
#include <vector>
#include <iostream>
#include <algorithm>
#include <thread>
#include <condition_variable>
#include <mutex>

using namespace std::chrono_literals;

constexpr std::size_t const N = NUM;
static std::vector<std::clock_t> tms(N);

static std::condition_variable cv;
static std::mutex cv_m;

static bool processed = false;

void wait_on_cv(std::size_t i) {
  std::unique_lock lk(cv_m);
  cv.wait(lk, []() { return processed; });

  tms[i] = std::clock();
}

int main(int, char **) {
  std::vector<std::thread> ths;

  for (int i = 0; i < N; ++i) {
    ths.push_back(std::thread([i]() { wait_on_cv(i); }));
  }
  std::this_thread::sleep_for(5s);

  auto start = std::clock();
  {
    std::lock_guard lk(cv_m);
    processed = true;
  }
  cv.notify_all();

  for (int i = 0; i < N; ++i) {
    ths[i].join();
  }

  for (int i = 0; i < N; ++i) {
    std::cout << i << '\t' << ((tms[i] - start) / double{CLOCKS_PER_SEC}) << '\n';
  }
}

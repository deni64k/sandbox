#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <unistd.h>

using namespace std::chrono_literals;

using pipe_t = int[2];

constexpr std::size_t const N = NUM;

static std::vector<std::clock_t> tms(N);
static std::vector<pipe_t> pipes(N);

void wait_on_pipe(std::size_t i) {
  char b;

  ::read(pipes[i][0], &b, sizeof(b));

  tms[i] = std::clock();
}

int main(int, char **) {
  std::vector<std::thread> ths;

  for (int i = 0; i < N; ++i) {
    if (::pipe(pipes[i]) == -1) {
      throw std::runtime_error("pipe failed");
    }

    ths.push_back(std::thread([i]() { wait_on_pipe(i); }));
  }
  std::this_thread::sleep_for(5s);

  auto start = std::clock();

  for (int i = 0; i < N; ++i) {
    static char b = 0xcc;
    ::write(pipes[i][1], &b, sizeof(b));
  }
  
  for (int i = 0; i < N; ++i) {
    ths[i].join();
  }

  for (int i = 0; i < N; ++i) {
    std::cout << i << '\t' << ((tms[i] - start) / double{CLOCKS_PER_SEC}) << '\n';
  }
}

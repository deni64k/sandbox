#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <system_error>

#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::chrono_literals;

using pipe_t = int[2];

constexpr std::size_t const N = NUM;

static std::array<std::clock_t, N> *tms_p;
static std::array<pipe_t, N>       *pipes_p;

void * create_shared_memory(std::size_t size) {
  // Our memory buffer will be readable and writable:
  int protection = PROT_READ | PROT_WRITE;

  // The buffer will be shared (meaning other processes can access it), but
  // anonymous (meaning third-party processes cannot obtain an address for it),
  // so only this process and its children will be able to use it:
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  // The remaining parameters to `mmap()` are not important for this use case,
  // but the manpage for `mmap` explains their purpose.
  auto *p = ::mmap(nullptr, size, protection, visibility, -1, 0);
  if (p == MAP_FAILED)
    throw std::system_error(errno, std::system_category(), "mmap failed");
  return p;
}

void wait_on_pipe(std::size_t i, int fd) {
  auto &tms = *tms_p;

  // std::cout << "proc0 " << i << ": " << tms_p << ": tms[i]=" << tms[i] << std::endl;

  char b;
  ::read(fd, &b, sizeof(b));

  // std::cout << "proc1 " << i << ": " << tms_p << ": tms[i]=" << tms[i] << std::endl;
  tms[i] = std::clock();
  // std::cout << "proc2 " << i << ": " << tms_p << ": tms[i]=" << tms[i] << std::endl;
}

int main(int, char **) {
  auto clktck = ::sysconf(_SC_CLK_TCK);
  std::cout << "_SC_CLK_TCK=\t" << clktck << std::endl;
  std::cout << "CLOCKS_PER_SEC=\t" << CLOCKS_PER_SEC << std::endl;

  std::vector<int> pids;

  std::size_t mem_size = 1024;
  tms_p   = static_cast<decltype(tms_p)>(create_shared_memory(mem_size));
  pipes_p = static_cast<decltype(pipes_p)>(create_shared_memory(mem_size));
  new (tms_p)   std::array<std::clock_t, N>;
  new (pipes_p) std::array<pipe_t, N>;

  auto &tms   = *tms_p;
  auto &pipes = *pipes_p;

  for (int i = 0; i < N; ++i) {
    if (::pipe(pipes[i]) == -1) {
      throw std::system_error(errno, std::system_category(), "pipe failed");
    }

    int pid = fork();
    if (pid == -1) {
      throw std::system_error(errno, std::system_category(), "fork failed");
    } else if (pid == 0) {
      ::close(pipes[i][1]);
      wait_on_pipe(i, pipes[i][0]);
      return 0;
    } else {
      ::close(pipes[i][0]);
      pids.push_back(pid);
    }
  }
  std::this_thread::sleep_for(5s);

  auto start = std::clock();

  for (int i = 0; i < N; ++i) {
    static char b = 0xcc;
    ::write(pipes[i][1], &b, sizeof(b));
  }
  
  ::wait(nullptr);

  struct tms t;
  auto finish = ::clock();
  ::times(&t);
  
  std::cout << "notifying took " << ((finish - start) / double{CLOCKS_PER_SEC}) << "s\n";
  std::cout << "\tutime\tstime\n";
  std::cout << "parent:\t"
            << (t.tms_utime / double{clktck}) << "s\t"
            << (t.tms_stime / double{clktck}) << "s\n";
  std::cout << "child:\t"
            << (t.tms_cutime / double{clktck}) << "s\t"
            << (t.tms_cstime / double{clktck}) << "s\n";
  for (int i = 0; i < N; ++i) {
    std::cout << i << '\t' << start << '\t' << (tms[i] / double{CLOCKS_PER_SEC}) << '\n';
    // std::cout << i << '\t' << start << '\t' << tms[i] << '\n';
    // std::cout << i << '\t' << (tms[i] - start/*) / double{CLOCKS_PER_SEC}*/) << '\n';
  }
}

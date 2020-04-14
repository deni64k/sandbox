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

#include <pthread.h>

using namespace std::chrono_literals;

constexpr std::size_t const N = NUM;

static bool *processed_p;

static std::array<std::clock_t, N> *tms_p;
static pthread_mutex_t *cv_m;
static pthread_cond_t  *cv;

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

void wait_on_cv(std::size_t i) {
  volatile auto &processed = *processed_p;
  auto &tms = *tms_p;

  // std::cout << "proc0 " << i << ": " << tms_p << ": tms[i]=" << tms[i] << std::endl;

  ::pthread_mutex_lock(cv_m);
  for (;;) {
    if (processed) {
      ::pthread_mutex_unlock(cv_m);
      break;
    }
    ::pthread_cond_wait(cv, cv_m);
  }
  ::pthread_mutex_unlock(cv_m);

  // std::cout << "proc1 " << i << ": " << tms_p << ": tms[i]=" << tms[i] << std::endl;
  tms[i] = std::clock();
  // std::cout << "proc2 " << i << ": " << tms_p << ": tms[i]=" << tms[i] << std::endl;
}

int main(int, char **) {
  auto clktck = ::sysconf(_SC_CLK_TCK);
  std::cout << "_SC_CLK_TCK=\t" << clktck << std::endl;
  std::cout << "CLOCKS_PER_SEC=\t" << CLOCKS_PER_SEC << std::endl;

  std::vector<int> pids;

  processed_p = static_cast<bool *>(create_shared_memory(sizeof(*processed_p)));
  cv    = static_cast<decltype(cv)>(create_shared_memory(sizeof(*cv)));
  cv_m  = static_cast<decltype(cv_m)>(create_shared_memory(sizeof(*cv_m)));
  tms_p = static_cast<decltype(tms_p)>(create_shared_memory(sizeof(*tms_p)));

  volatile auto &processed = *processed_p;

  ::pthread_mutexattr_t m_attr;
  ::pthread_mutexattr_init(&m_attr);
  ::pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
  ::pthread_mutex_init(cv_m, &m_attr);

  ::pthread_condattr_t cv_attr;
  ::pthread_condattr_init(&cv_attr);
  ::pthread_condattr_setpshared(&cv_attr, PTHREAD_PROCESS_SHARED);
  ::pthread_cond_init(cv, &cv_attr);

  new (tms_p)   std::array<std::clock_t, N>;
  auto &tms       = *tms_p;

  for (int i = 0; i < N; ++i) {
    int pid = fork();
    if (pid == -1) {
      throw std::system_error(errno, std::system_category(), "fork failed");
    } else if (pid == 0) {
      wait_on_cv(i);
      return 0;
    } else {
      pids.push_back(pid);
    }
  }
  std::this_thread::sleep_for(5s);

  auto start = std::clock();

  ::pthread_mutex_lock(cv_m);
  processed = true;
  ::pthread_mutex_unlock(cv_m);
  ::pthread_cond_broadcast(cv);
  
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

  ::pthread_mutex_destroy(cv_m);
  ::pthread_condattr_destroy(&cv_attr);
  ::pthread_cond_destroy(cv);
}

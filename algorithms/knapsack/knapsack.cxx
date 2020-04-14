#include <ctime>
#include <iostream>
#include <map>
#include <tuple>
#include <vector>

#define timeit(x) { \
    auto const __start = std::clock();         \
    x;                                         \
    auto const __interval = std::clock() - __start;                     \
    std::cerr << "timeit: " << static_cast<double>(__interval) / CLOCKS_PER_SEC << " s\n"; \
}

typedef std::tuple<int, int> Item;

std::map<std::string, long int> memo;

long int knapsack_recursive(int capacity, std::vector<Item> const &items, int index = 0) {
  if (index >= items.size())
    return 0;
  
  int v, w;
  std::tie(v, w) = items[index];

  if (capacity < w)
    return knapsack_recursive(capacity, items, index+1);
  if (capacity == w)
    return v;

  char key_raw[45];
  auto n = ::snprintf(key_raw, sizeof(key_raw), "%d:%d", capacity, index);
  std::string key(key_raw);
  auto iter = memo.lower_bound(key);
  if (iter->first == key) {
    return iter->second;
  }

  auto answer = std::max(knapsack_recursive(capacity, items, index+1),
                         knapsack_recursive(capacity - w, items, index+1) + v);

  memo.emplace_hint(iter, key, answer);

  return answer;
}

long int knapsack_table(int capacity, std::vector<Item> const &items) {
  std::vector<long int> prev_memo(capacity+1, 0), memo(capacity+1);

  for (int i = 0; i < items.size(); ++i) {
    for (int c = 0; c <= capacity; ++c) {
      int v, w;
      std::tie(v, w) = items[i];

      if (c-w < 0) {
        memo[c] = prev_memo[c];
        continue;
      }

      memo[c] = std::max(prev_memo[c], prev_memo[c-w] + static_cast<long int>(v));
    }
    memo.swap(prev_memo);
  }

  return prev_memo[capacity];
}

int main(int argc, char **argv) {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  int capacity, n;
  std::cin >> capacity >> n;

  std::vector<Item> items;
  items.reserve(n);
  for (int i = 0; i < n; ++i) {
    int v, w;
    std::cin >> v >> w;
    items.emplace_back(v, w);
  }

  long int answer;

  timeit(
    answer = knapsack_table(capacity, items);
  );
  std::cout << "knapsack_table=" << answer << '\n';

  timeit(
    answer = knapsack_recursive(capacity, items);
  );
  std::cout << "knapsack_recursive=" << answer << '\n';
}

// knapsack1.txt    -- 2493893 -- table: 0.002047s recursive: 1.90566s
// knapsack_big.txt -- 4243395 -- table: 6.87643s  recursive: 32.2932s

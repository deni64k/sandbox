#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>

struct BoundEdge {
  BoundEdge(int v_, int l_)
  : v(v_), l(l_) {}

  int v, l;
};

typedef std::vector<std::vector<BoundEdge>> Graph;

int bellman_ford(Graph const &graph, int s) {
  auto const n = graph.size();
  std::vector<int> memo(n, std::numeric_limits<int>::max());

  memo[s] = 0;

  for (int i = 1; i < graph.size(); ++i) {
    auto alt = std::numeric_limits<int>::max();
    for (int j = 0; j < graph.size(); ++j) {
      if (memo[j] == std::numeric_limits<int>::max())
        continue;

      for (auto const &edge : graph[j]) {
        if (memo[j] + edge.l < memo[edge.v]) {
          memo[edge.v] = memo[j] + edge.l;
        }
      }
    }
  }

  for (int i = 0; i < graph.size(); ++i) {
    for (auto const &edge : graph[i]) {
      if (memo[i] + edge.l < memo[edge.v]) {
        std::cout << "Negative cycle detected\n";
        return -1;
      }
    }
  }

  auto mn = std::numeric_limits<int>::max();
  for (int i = 0; i < n; ++i) {
    if (i == s)
      continue;
    mn = std::min(mn, memo[i]);
  }
  return mn;
}

int main(int argc, char *argv[]) {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  int n, m;
  std::cin >> n >> m;

  Graph graph(n);
  for (int i = 0; i < m; ++i) {
    int v, w, l;
    std::cin >> v >> w >> l;
    --v;
    --w;

    auto &edges = graph[v];
    edges.emplace_back(w, l);
  }

  int min = std::numeric_limits<int>::max();
  for (int i = 0; i < graph.size(); i+=2) {
    std::cout << "source=" << i << '\n';
    min = std::min(min, bellman_ford(graph, i));
  }

  std::cout << "min=" << min << '\n';
  
  return 0;
}

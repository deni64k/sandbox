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

int ffloyd_warshall(Graph const &graph) {
  auto const n = graph.size();
  auto const zero = std::numeric_limits<int>::max();
  std::vector<std::vector<int>> const memo_zero(n, std::vector<int>(n, zero));

  std::vector<std::vector<int>> memo(n);
  std::vector<std::vector<int>> memo_prev(memo_zero);

  for (int i = 0; i < n; ++i) {
    memo_prev[i][i] = 0;
    auto const &edges = graph[i];
    for (auto const &edge : edges) {
      memo_prev[i][edge.v] = edge.l;
    }
  }

  for (int k = 0; k < n; ++k) {
    memo = memo_zero;
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        auto const l_ik = memo_prev[i][k];
        auto const l_kj = memo_prev[k][j];

        auto alt = memo_prev[i][j];
        if (l_ik != zero && l_kj != zero) {
          alt = std::min(alt, l_ik + l_kj);
        }
        
        memo[i][j] = alt;
      }
    }
    memo.swap(memo_prev);
  }

  for (int i = 0; i < n; ++i) {
    assert(memo_prev[i][i] == 0 && "Negative cycle detected");
  }

  auto mn = std::numeric_limits<int>::max();
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      if (i == j)
        continue;
      mn = std::min(mn, memo[i][j]);
    }
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

  auto const min = ffloyd_warshall(graph);

  std::cout << "min=" << min << '\n';
  
  return 0;
}

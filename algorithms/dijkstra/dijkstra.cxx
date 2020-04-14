#include <cmath>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
#include <queue>
#include <map>
#include <set>

struct BoundEdge {
  BoundEdge(int v_, int r_)
  : v(v_)
  , r(r_)
  {}
  int v, r;
};

typedef std::vector<std::vector<BoundEdge>> Graph;

struct Dist {
  Dist(int v_, int d_)
    : v(v_)
    , d(d_)
  {}
  int v, d;
};

bool operator > (Dist const &l, Dist const &r) {
  return l.d > r.d;
}

auto dijkstra(Graph const &graph, int s) -> void {
  auto const n = graph.size();
  std::vector<long long int> dists(n, -1);
  dists[s] = 0;

  std::priority_queue<Dist, std::vector<Dist>, std::greater<Dist>> queue;
  queue.emplace(s, dists[s]);

  while (!queue.empty()) {
    Dist min_dist = queue.top();
    queue.pop();

    int const u = min_dist.v;
    if (dists[u] < min_dist.d) {
      continue;
    }

    auto const &neighbours = graph[u];
    for (auto iter = neighbours.cbegin(); iter != neighbours.cend(); ++iter) {
      int v = iter->v;
      int alt = dists[u] + iter->r;
      if (alt < dists[v] || dists[v] < 0) {
        dists[v] = alt;
        queue.emplace(v, dists[v]);
      }
    }
  }
    
  for (int i = 0; i < n-1; i++) {
    if (i == s)
      continue;
    printf("%lld ", dists[i]);
  }
  printf("%lld\n", dists[n-1]);
}

int main() {
  int q;
  scanf("%d", &q);
    
  for (int i = 0; i < q; i++) {
    // Read N and M.
    int n, m;
    scanf("%d %d", &n, &m);
    
    // Read edges.
    Graph graph(n);
    for (int i = 0; i < m; i++) {
      int u, v, r;
      scanf("%d %d %d", &u, &v, &r);
      u--;
      v--;
      graph[u].push_back(BoundEdge(v, r));
      graph[v].push_back(BoundEdge(u, r));
    }

    // Read the source vertex.
    int s;
    scanf("%d", &s);
    s--;

    fprintf(stderr, "n=%d, m=%d, s=%d\n", n, m, s);

    dijkstra(graph, s);
  }

  return 0;
}

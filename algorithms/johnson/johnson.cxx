#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdio>
#include <deque>
#include <queue>
#include <vector>

/*

In this assignment you will implement one or more algorithms for the all-pairs shortest-path problem. Here are data files describing three graphs:

g1.txt
g2.txt
g3.txt
The first line indicates the number of vertices and edges, respectively.
Each subsequent line describes an edge (the first two numbers are its tail and head, respectively) and its length (the third number).
NOTE: some of the edge lengths are negative.
NOTE: These graphs may or may not have negative-cost cycles.

Your task is to compute the "shortest shortest path". Precisely, you must first identify which, if any, of the three graphs have no negative cycles. For each such graph, you should compute all-pairs shortest paths and remember the smallest one (i.e., compute minu,v∈Vd(u,v), where d(u,v) denotes the shortest-path distance from u to v).

If each of the three graphs has a negative-cost cycle, then enter "NULL" in the box below. If exactly one graph has no negative-cost cycles, then enter the length of its shortest shortest path in the box below. If two or more of the graphs have no negative-cost cycles, then enter the smallest of the lengths of their shortest shortest paths in the box below.

OPTIONAL: You can use whatever algorithm you like to solve this question. If you have extra time, try comparing the performance of different all-pairs shortest-path algorithms!

OPTIONAL: Here is a bigger data set to play with.

large.txt
For fun, try computing the shortest shortest path of the graph in the file above.

*/

// shortest=-19
// go run g.go  1548.46s user 10.51s system 96% cpu 26:47.92 total

struct Edge {
  Edge(int v_, int r_)
    : v(v_)
    , r(r_)
  {}
  int v, r;
};

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

typedef std::pair<std::vector<int>, bool> bellman_ford_result_type;

auto bellman_ford(int n, int m, std::deque<std::vector<Edge>> const &graph, int s)
  -> bellman_ford_result_type {
  std::deque<std::vector<Edge>> graph_rev(graph.size());
  for (int v = 0; v < graph.size(); ++v) {
    auto const &edges = graph[v];
    for (auto it = edges.cbegin(); it != edges.cend(); ++it) {
      graph_rev[it->v].push_back(Edge(v, it->r));
    }
  }

  std::vector<int> memo(n+1);
  std::vector<int> prev_memo(n+1, INT_MAX);
  prev_memo[s] = 0;

  for (int i = 1; i < n + 1; ++i) {
    std::fill(memo.begin(), memo.end(), INT_MAX);

    for (int w = s; w >= 0; w--) {
      auto min_path = prev_memo[w];
      auto const &neighbours = graph_rev[w];
      for (auto iter = neighbours.cbegin(); iter != neighbours.cend(); ++iter) {
        auto const &e = *iter;
        if (prev_memo[e.v] == INT_MAX)
          continue;
        min_path = std::min(min_path, prev_memo[e.v] + e.r);
      }
      memo[w] = min_path;
    }
    prev_memo.swap(memo);
  }

  std::vector<int> scores(n);
  for (int i = 0; i < n; i++) {
    if (prev_memo[i] != memo[i]) {
      return bellman_ford_result_type(std::vector<int>(), false);
    }
    scores[i] = memo[i];
  }
  return bellman_ford_result_type(scores, true);
}

auto dijkstra(int n, int m,
              std::deque<std::vector<Edge>> const &edges, int s)
  -> std::vector<int> {
  std::vector<int> dists(n, INT_MAX);
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

    auto const &neighbours = edges[u];
    for (auto iter = neighbours.cbegin(); iter != neighbours.cend(); ++iter) {
      int v = iter->v;
      int alt = dists[u] + iter->r;
      if (alt < dists[v]) {
        dists[v] = alt;
        queue.emplace(v, alt);
      }
    }
  }

  return dists;
}

auto johnson() -> void {
  // Read N and M.
  int n, m;
  scanf("%d %d", &n, &m);
    
  // Read edges.
  std::deque<std::vector<Edge>> graph(n);
  for (int i = 0; i < m; i++) {
    int u, v, r;
    scanf("%d %d %d", &u, &v, &r);
    u--;
    v--;
    graph[u].push_back(Edge(v, r));
  }

  // Add a helper vertix and bind it with every vertices of the graph.
  {
    std::vector<Edge> helper_edges;
    helper_edges.reserve(n);
    for (int v = 0; v < n; v++) {
      helper_edges.push_back(Edge(v, 0));
    }
    graph.push_back(std::move(helper_edges));
  }

  // Run Bellman-Ford algorithm.
  auto bf_result = bellman_ford(n, m, graph, n);
  if (!bf_result.second) {
    fprintf(stderr, "negative cycle detected\n");
    return;
  }
  auto scores = std::move(bf_result.first);

  // Remove the helper vertix.
  graph.erase(graph.begin() + n);

  // Reweight the graph to get rid of negative edges.
  for (int u = 0; u < n; u++) {
    for (auto it = graph[u].begin(); it != graph[u].end(); ++it) {
      // printf("u=%d, v=%d");
      it->r += scores[u] - scores[it->v];
      assert(it->r >= 0);
    }
  }

  // Run Dijstra algrotihm for each pair of vertices and find the shortest path.
  int shortest_path = INT_MAX;
  for (int u = 0; u < n; u++) {
    auto const dists = dijkstra(n, m, graph, u);
    for (int v = 0; v < n; v++) {
      auto const d = dists[v];
      if (d == INT_MAX)
        continue;
      shortest_path = std::min(shortest_path,
                               d + scores[v] - scores[u]);
    }
    if (u % 10 == 0) {
      printf("dijkstra progress: %f\r", float(u)/float(n));
    }
  }
  printf("dijkstra progress: %f\n", 1.0f);

  printf("Shortest path is %i\n", shortest_path);
}

int main() {
  johnson();

  return 0;
}

// % g++ -std=c++14 -Ofast -mtune=native johnson.cxx -o johnson && cat large.txt | time ./johnson
// Shortest path is -60.999500
// ./johnson  412.79s user 4.01s system 96% cpu 7:12.25 total

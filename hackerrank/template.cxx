#include <bits/stdc++.h>

#if 1
# define DEBUG(x)  {x;}
# define TRACEV(v) {std::cerr << __func__ << ": " << __LINE__ << ": " << #v << '=' << v << '\n';}
# define TRACE(v)  {std::cerr << __func__ << ": " << __LINE__ << '\n';}
#else
# define DEBUG(x)
# define TRACEV(v)
# define TRACE(v)
#endif

using namespace std;

struct WeightedVertex {
  WeightedVertex(int v, int w)
  : V(v), W(w) {}

  int V, W;
};
typedef vector<vector<WeightedVertex>> WeightedGraph;

typedef int Vertex;
typedef vector<vector<Vertex>> Graph;

typedef pair<int, int> Edge;

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  int N;

  cin >> N;
  while (N--) {
    ;
  }

  return 0;
}

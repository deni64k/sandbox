#include <bits/stdc++.h>

using namespace std;

/*
 * Complete the shortestPath function below.
 */
using cost_type = unsigned;
using location = pair<unsigned, unsigned>;

namespace std {
  template <>
  struct hash<location> {
    size_t operator () (location const& loc) const noexcept {
      size_t const h1 = std::hash<unsigned>{}(loc.first);
      size_t const h2 = std::hash<unsigned>{}(loc.second);
      return h1 ^ (h2 << 1);
    }
  };
}

constexpr auto absdiff(cost_type x, cost_type y) {
  return x > y ? x - y : y - x;
}
constexpr auto mdist(location const& lhs, location const& rhs) {
  return absdiff(lhs.first, rhs.first) + absdiff(lhs.second, rhs.second);
}

using weighted_location = location;
struct weighted_location_cmp {
  vector<vector<size_t>> const& regions;
  vector<cost_type> const& weights;

  auto operator () (location const& lhs,
                    location const& rhs) noexcept {
    auto const lhs_region = regions[lhs.first][lhs.second];
    auto const rhs_region = regions[rhs.first][rhs.second];
    return weights[lhs_region] > weights[rhs_region];
  }
};

auto get_neighbors(vector<vector<int>> const& a, location const& loc)
-> pair<std::array<location, 4>, unsigned> {
  array<location, 4> neighbors;
  unsigned neighbors_i = 0;
  if (loc.first > 0)
    neighbors[neighbors_i++] = location{loc.first - 1, loc.second};
  if (loc.second > 0)
    neighbors[neighbors_i++] = location{loc.first, loc.second - 1};
  if (loc.first < a.size() - 1)
    neighbors[neighbors_i++] = location{loc.first + 1, loc.second};
  if (loc.second < a[0].size() - 1)
    neighbors[neighbors_i++] = location{loc.first, loc.second + 1};
  return {neighbors, neighbors_i};
}

vector<int> shortestPath(vector<vector<int>> const& a, vector<array<int, 4>> const& queries) {
  vector<int> costs(queries.size(), numeric_limits<int>::max());

  map<location, vector<size_t>> grouped_queries;
  for (size_t query_i = 0; query_i < queries.size(); ++query_i) {
    auto const& query = queries[query_i];
    location start{query[0], query[1]};
    auto iter = grouped_queries.find(start);
    if (iter == grouped_queries.end())
      grouped_queries.emplace(start, vector<size_t>{query_i});
    else
      iter->second.push_back(query_i);
  }

  auto regions = vector<vector<size_t>>(a.size(),
                                        vector<size_t>(a[0].size(), 0));
  map<size_t, set<location>> edges;
  size_t region_i = 0;
  for (int r = 0; r < a.size(); ++r) {
    for (int c = 0; c < a[0].size(); ++c) {
      if (a[r][c] == 0 && regions[r][c] == 0) {
        queue<location> q;
        q.push({r, c});
        while (!q.empty()) {
          auto loc = q.front();
          q.pop();

          regions[loc.first][loc.second] = region_i;

          auto const [neighbors, num_neighbors] = get_neighbors(a, loc);
          for (int i = 0; i < num_neighbors; ++i) {
            auto const& neighbor = neighbors[i];
            if (regions[neighbor.first][neighbor.second] != 0)
              continue;

            if (a[neighbor.first][neighbor.second] == 0) {
              q.push(neighbor);
            } else
              edges[region_i].insert(neighbor);
          }
        }
        ++region_i;
      }
    }
  }
  for (int r = 0; r < a.size(); ++r) {
    for (int c = 0; c < a[0].size(); ++c) {
      if (a[r][c] != 0 && regions[r][c] == 0) {
        regions[r][c] = region_i;
        auto const [neighbors, num_neighbors] = get_neighbors(a, location{r, c});
        for (int i = 0; i < num_neighbors; ++i) {
          auto const& neighbor = neighbors[i];
          edges[region_i].insert(neighbor);
          if (a[neighbor.first][neighbor.second] == 0) {
            auto const neighbor_region = regions[neighbor.first][neighbor.second];
            copy(cbegin(edges[neighbor_region]), cend(edges[neighbor_region]),
                 inserter(edges[region_i], edges[region_i].end()));
          }
        }
        ++region_i;
      }
    }
  }

  for (auto const& grouped_query : grouped_queries) {
    location const& start = grouped_query.first;
    auto const start_region = regions[start.first][start.second];
    vector<cost_type> ws(region_i, numeric_limits<cost_type>::max());
    ws[start_region] = a[start.first][start.second];

    vector<weighted_location> opened = {{start}};

    while (!opened.empty()) {
      auto const loc = opened.front();
      auto const loc_region = regions[loc.first][loc.second];

      pop_heap(opened.begin(), opened.end(), weighted_location_cmp{regions, ws});
      opened.pop_back();

      auto const w = ws[loc_region];
      // if (loc == finish && w != numeric_limits<int>::max()) {
      //   costs[query_i] = w;
      //   // cerr << "answer with ws[" << loc.first << "][" << loc.second << "] = "
      //   //      << ws[loc.first][loc.second] << '\n';
      //   break;
      // }

      auto const all_neighbors = edges[loc_region];

      std::cerr << "loc= " << loc.first << ',' << loc.second << '\n';
      std::cerr << "loc_region= " << loc_region << '\n';

      for (auto const& neighbor : all_neighbors) {
        auto const neighbor_cost = a[neighbor.first][neighbor.second];
        std::cerr << "neighbor: " << neighbor.first << ',' << neighbor.second << '\n';
        cost_type const new_cost = w + neighbor_cost;
        auto const neighbor_region = regions[neighbor.first][neighbor.second];
        cost_type& current_cost = ws[neighbor_region];
        std::cerr << "new_cost: " << new_cost << "; current_cost: " << current_cost
                  << "; current_cost <= new_cost = " << (current_cost <= new_cost) << '\n';
        if (current_cost <= new_cost)
          continue;

        // if (new_cost <= numeric_limits<int>::max())
        //   current_cost = new_cost;

        current_cost = new_cost;

        auto iter = opened.begin();
        for (; iter != opened.end(); ++iter) {
          if (*iter == neighbor) {
            // iter->second = current_cost;
            break;
          }
        }
        if (iter == opened.end()) {
          opened.push_back(neighbor);
          iter = opened.end();
        } else
          ++iter;
        push_heap(opened.begin(), iter, weighted_location_cmp{regions, ws});
      }
    }

    for (auto const query_i : grouped_query.second) {
      auto const& query = queries[query_i];
      location finish{query[2], query[3]};
      auto const finish_region = regions[finish.first][finish.second];
      costs[query_i] = ws[finish_region];
    }
  }

  //std::cerr << "costs.size(): " << costs.size() << '\n';
  return costs;
}

int main()
{
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  int n, m;
  cin >> n >> m;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  vector<vector<int>> a(n);
  for (int a_row_itr = 0; a_row_itr < n; a_row_itr++) {
      a[a_row_itr].resize(m);

      for (int a_column_itr = 0; a_column_itr < m; a_column_itr++) {
          cin >> a[a_row_itr][a_column_itr];
      }

      cin.ignore(numeric_limits<streamsize>::max(), '\n');
  }

  int q;
  cin >> q;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  vector<array<int, 4>> queries(q);
  for (int queries_row_itr = 0; queries_row_itr < q; queries_row_itr++) {
      for (int queries_column_itr = 0; queries_column_itr < 4; queries_column_itr++) {
          cin >> queries[queries_row_itr][queries_column_itr];
      }

      cin.ignore(numeric_limits<streamsize>::max(), '\n');
  }

  vector<int> result = shortestPath(a, queries);

  ofstream fout(getenv("OUTPUT_PATH"));
  for (int result_itr = 0; result_itr < result.size(); result_itr++) {
      fout << result[result_itr];

      if (result_itr != result.size() - 1) {
          fout << "\n";
      }
  }

  fout << "\n";

  fout.close();

  return 0;
}

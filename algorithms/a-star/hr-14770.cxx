#include <bits/stdc++.h>

#if 0
# define D(...) __VA_ARGS__
#else
# define D(...)
#endif

// namespace std {
// template <typename... Ts> struct make_void { typedef void type;};
// template <typename... Ts> using void_t = typename make_void<Ts...>::type;
// template <typename T> using remove_cvref_t = typename remove_cv<typename remove_reference<T>::type>::type;
// }

using namespace std;

using Weight = unsigned;

struct Location {  
  std::size_t row;
  std::size_t col;

  friend std::istream& operator >> (std::istream& is, Location& rhs) {
    return is >> rhs.row >> rhs.col;
  }
  friend std::ostream& operator << (std::ostream& os, Location const& rhs) {
    return os << rhs.row << ", " << rhs.col;
  }
  friend constexpr bool
  operator < (Location const& lhs, Location const& rhs) noexcept {
    return std::make_pair(lhs.row, lhs.col) < std::make_pair(rhs.row, rhs.col);
  }
  friend constexpr bool
  operator <= (Location const& lhs, Location const& rhs) noexcept {
    return std::make_pair(lhs.row, lhs.col) <= std::make_pair(rhs.row, rhs.col);
  }
  friend constexpr bool
  operator > (Location const& lhs, Location const& rhs) noexcept {
    return std::make_pair(lhs.row, lhs.col) > std::make_pair(rhs.row, rhs.col);
  }
  friend constexpr bool
  operator >= (Location const& lhs, Location const& rhs) noexcept {
    return std::make_pair(lhs.row, lhs.col) >= std::make_pair(rhs.row, rhs.col);
  }
  friend constexpr bool
  operator == (Location const& lhs, Location const& rhs) noexcept {
    return std::make_pair(lhs.row, lhs.col) == std::make_pair(rhs.row, rhs.col);
  }
};

template <typename T>
struct Map {
  using value_type = T;

  std::size_t rows;
  std::size_t cols;
  std::unique_ptr<T[]> nodes;

  Map(std::size_t rows_, std::size_t cols_):
    rows{rows_},
    cols{cols_},
    nodes{std::make_unique<T[]>(rows * cols)}
  {}

  Map(Map const& other):
    rows{other.rows},
    cols{other.cols},
    nodes{std::make_unique<T[]>(rows * cols)}
  {
    std::copy(&other.nodes[0], &other.nodes[rows * cols], &nodes[0]);
  }
  Map& operator = (Map const& other) {
    rows = other.rows;
    cols = other.cols;
    nodes = std::make_unique<T[]>(rows * cols);
    uninitialized_copy_n(other.nodes.get(), rows * cols, &nodes[0]);
    return *this;
  }

  Map(Map&&) = default;
  Map& operator = (Map&&) = default;

  void fill(T w) noexcept {
    uninitialized_fill_n(nodes.get(), rows * cols, w);
  }

  struct Row {
    T* nodes;

    constexpr T& operator [] (std::size_t col) noexcept {
      return nodes[col];
    }
    constexpr T const& operator [] (std::size_t col) const noexcept {
      return nodes[col];
    }
  };

  Row operator [] (std::size_t row) noexcept {
    return {nodes.get() + row * cols};
  }
  Row operator [] (std::size_t row) const noexcept {
    return {nodes.get() + row * cols};
  }
  T& operator [] (Location const& loc) const noexcept {
    return operator [] (loc.row)[loc.col];
  }
};

namespace std {
  template <>
  struct hash<Location> {
    [[gnu::optimize("O3")]]
    size_t operator () (Location const& loc) const noexcept {
      size_t const h1 = std::hash<size_t>{}(loc.row);
      size_t const h2 = std::hash<size_t>{}(loc.col);
      return h1 ^ (h2 << 1);
    }
  };
}

template <typename T>
[[gnu::optimize("O3")]]
constexpr T
absdiff(T x, T y) {
#if defined(__GNUC__) || defined(__clang__)
  T result;
  if (__builtin_sub_overflow(x, y, &result))
    return y - x;
  return result;
#else
  return x > y ? x - y : y - x;
#endif
}

[[gnu::optimize("O3")]]
constexpr auto
mdist(Location const& lhs, Location const& rhs) {
  return absdiff(lhs.row, rhs.row) + absdiff(lhs.col, rhs.col);
}

template <typename Container, typename = void_t<>>
struct addressable_by_location: false_type {};

template <typename Container>
struct addressable_by_location<
  Container,
  void_t<decltype(declval<Container>()[declval<Location const&>()])>>
: true_type {};

template <typename Container, typename = void_t<>>
struct getloc_impl {};

template <typename Container>
struct getloc_impl<
    Container,
    enable_if_t<!addressable_by_location<Container>::value>> {
  [[gnu::optimize("O3")]]
  inline decltype(auto)
  operator () (Container&& c, Location const& loc) noexcept {
    return c[loc.row][loc.col];
  }
};

template <typename Container>
struct getloc_impl<
    Container,
    enable_if_t<addressable_by_location<Container>::value>> {
  [[gnu::optimize("O3")]]
  inline decltype(auto)
  operator () (Container&& c, Location const& loc) noexcept {
    return c[loc];
  }
};

template <typename Container>
[[gnu::optimize("O3")]]
inline decltype(auto)
getloc(Container&& c, Location const& loc) noexcept {
  return getloc_impl<Container>{}(std::forward<Container>(c), loc);
}

struct WeightedLocation {
  Weight   weight;
  Location parent;
};

template <typename T> struct HeapCmp {};

template <> struct HeapCmp<Weight> {
  Map<Weight> const& weights;

  [[gnu::optimize("O3")]]
  auto operator () (Location const& lhs,
                    Location const& rhs) noexcept {
    return getloc(weights, lhs) > getloc(weights, rhs);
  }
};

template <> struct HeapCmp<WeightedLocation> {
  Map<WeightedLocation> const& weights;

  [[gnu::optimize("O3")]]
  auto operator () (Location const& lhs,
                    Location const& rhs) noexcept {
    return getloc(weights, lhs).weight > getloc(weights, rhs).weight;
  }
};

template <typename Container>
[[gnu::optimize("O3")]]
inline auto
make_heap_cmp(Container const& cont) {
  using result_type = HeapCmp<typename remove_cvref_t<Container>::value_type>;
  return result_type{cont};
}

[[gnu::optimize("O3")]]
inline auto
get_neighbors(Map<Weight> const& a, Location const& loc) noexcept {
  array<Location, 4> neighbors;
  size_t neighbors_i = 0;
  if (loc.row > 0)
    neighbors[neighbors_i++] = Location{loc.row - 1, loc.col};
  if (loc.col > 0)
    neighbors[neighbors_i++] = Location{loc.row, loc.col - 1};
  if (loc.row < a.rows - 1)
    neighbors[neighbors_i++] = Location{loc.row + 1, loc.col};
  if (loc.col < a.cols - 1)
    neighbors[neighbors_i++] = Location{loc.row, loc.col + 1};
  return make_pair(neighbors, neighbors_i);
}

[[gnu::optimize("O3")]]
inline vector<int>
shortest_paths(Map<Weight> const& a,
               vector<array<Location, 2>> const& queries) {
  auto answers = vector<int>(queries.size(), numeric_limits<int>::max());
  //auto highway = Map<WeightedLocation>(a.rows, a.cols);
  auto weights = Map<Weight>(a.rows, a.cols);

  unordered_map<Location, set<Location>> all_edges;
  for (size_t r = 0; r < a.rows; ++r) {
    for (size_t c = 0; c < a.cols; ++c) {
      auto const loc = Location{r, c};
      auto const [neighbors, num_neighbors] = get_neighbors(a, loc);
      all_edges.emplace(loc,
                        set<Location>(neighbors.cbegin(),
                                      neighbors.cbegin() + num_neighbors));
    }
  }

  map<Location, vector<size_t>> grouped_queries;
  for (size_t query_i = 0; query_i < queries.size(); ++query_i) {
    Location const& start = queries[query_i][0];
    auto iter = grouped_queries.find(start);
    if (iter == grouped_queries.end())
      grouped_queries.emplace(start, vector<size_t>{query_i});
    else
      iter->second.push_back(query_i);
  }

  /*for (size_t r = 0; r < a.rows; ++r) {
    for (size_t c = 0; c < a.cols; ++c) {
      highway[r][c] = WeightedLocation{numeric_limits<Weight>::max(), {r, c}};
    }
  }
  auto const topleft = Location{0, 0};
  auto const bottomright = Location{a.rows-1, a.cols-1};
  getloc(highway, topleft).weight = getloc(a, topleft);
  {
    vector<Location> opened = {{topleft}};

    while (!opened.empty()) {
      auto const loc = opened.front();

      pop_heap(opened.begin(), opened.end(), make_heap_cmp(highway));
      opened.pop_back();

      auto const w = getloc(highway, loc).weight;
      if (loc == bottomright)
        break;

      auto const& neighbors = all_edges[loc];
      for (auto const& neighbor : neighbors) {
        Weight const neighbor_cost = getloc(a, neighbor);
        Weight const new_weight = w + neighbor_cost;
        Weight& current_weight = getloc(highway, neighbor).weight;
        Location& current_parent = getloc(highway, neighbor).parent;
        if (current_weight <= new_weight)
          continue;

        current_weight = new_weight;
        current_parent = loc;

        auto iter = opened.rbegin();
        for (; iter != opened.rend() && !(*iter == neighbor); ++iter);
        if (iter == opened.rend()) {
          opened.push_back(neighbor);
          iter = opened.rbegin();
        }
        push_heap(opened.begin(), iter.base(), make_heap_cmp(highway));
      }
    }
  }*/

  for (auto const& grouped_query : grouped_queries) {
    Location const& start = grouped_query.first;
    D(std::cerr << "start= " << start << '\n');
    weights.fill(numeric_limits<Weight>::max());
    getloc(weights, start) = getloc(a, start);

    auto const& query_targets = grouped_query.second;
    auto query_target = query_targets.cbegin();
    Location target = queries[*query_target][1];

    vector<Location> opened = {{start}};
    while (!opened.empty()) {
      auto const loc = opened.front();

      pop_heap(opened.begin(), opened.end(), make_heap_cmp(weights));
      opened.pop_back();

      D(std::cerr << "loc= " << loc << '\n');

      auto const w = getloc(weights, loc);
      if (loc == target) {
        D(std::cerr << "target= " << target << '\n');
        answers[*query_target] = w;
        while (++query_target != query_targets.cend()) {
          target = queries[*query_target][1];
          D(std::cerr << "target= " << target << '\n');
          auto const w = getloc(weights, target);
          if (w == numeric_limits<Weight>::max())
            goto continue_grouped_query;
          else
            answers[*query_target] = w;
        }
        if (query_target == query_targets.cend()) {
          goto exit_grouped_query;
        }
      }

continue_grouped_query:
      auto const& neighbors = all_edges[loc];

      for (auto const& neighbor : neighbors) {
        D(std::cerr << "neighbor: " << neighbor << '\n');
        Weight const neighbor_cost = getloc(a, neighbor);
        Weight const new_cost = w + neighbor_cost;
        Weight& current_cost = getloc(weights, neighbor);
        D(std::cerr << "new_cost: " << new_cost << "; current_cost: " << current_cost
                    << "; current_cost <= new_cost = " << (current_cost <= new_cost) << '\n');
        if (current_cost <= new_cost)
          continue;

        current_cost = new_cost;

        auto iter = opened.rbegin();
        for (; iter != opened.rend() && !(*iter == neighbor); ++iter);
        if (iter == opened.rend()) {
          opened.push_back(neighbor);
          iter = opened.rbegin();
        }
        push_heap(opened.begin(), iter.base(), make_heap_cmp(weights));
      }
    }

exit_grouped_query: ;
    // for (auto const query_i : grouped_query.second) {
    //   auto const& query = queries[query_i];
    //   Location const& finish = query[1];
    //   answers[query_i] = getloc(weights, finish);
    // }
  }

  //std::cerr << "costs.size(): " << costs.size() << '\n';
  return answers;
}

int main(int, char**) {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  int rows, cols;
  cin >> rows >> cols;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  auto a = Map<Weight>(rows, cols);
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      cin >> a[r][c];
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
  }

  int q;
  cin >> q;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  vector<array<Location, 2>> queries(q);
  for (int i = 0; i < q; ++i) {
    cin >> queries[i][0]
        >> queries[i][1];
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
  }

  auto results = shortest_paths(a, queries);

  ofstream fout(getenv("OUTPUT_PATH"));
  for (auto&& result : results) {
    static string const nl = "\n";
    fout << result << nl;
  }
  fout.flush();
  fout.close();

  return 0;
}

#include <tuple>
#include <limits>
#include <memory>
#include <ostream>
#include <istream>

#if 0
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#if 0
#define TRACE(x) std::cerr << #x << x << std::endl
#else
#define TRACE(x)
#endif

using weight_type = std::size_t;

struct Location {  
  std::size_t row;
  std::size_t col;

  friend std::istream& operator >> (std::istream& is, Location& o) {
    return is >> o.row >> o.col;
  }
  friend std::ostream& operator << (std::ostream& os, Location const& o) {
    return os << o.row << ", " << o.col;
  }
  friend bool operator < (Location const& lhs, Location const& rhs) {
    return std::tuple{lhs.row, lhs.col} < std::tuple{rhs.row, rhs.col};
  }
  friend bool operator <= (Location const& lhs, Location const& rhs) {
    return std::tuple{lhs.row, lhs.col} <= std::tuple{rhs.row, rhs.col};
  }
  friend bool operator > (Location const& lhs, Location const& rhs) {
    return std::tuple{lhs.row, lhs.col} > std::tuple{rhs.row, rhs.col};
  }
  friend bool operator >= (Location const& lhs, Location const& rhs) {
    return std::tuple{lhs.row, lhs.col} >= std::tuple{rhs.row, rhs.col};
  }
  friend bool operator == (Location const& lhs, Location const& rhs) {
    return std::tuple{lhs.row, lhs.col} == std::tuple{rhs.row, rhs.col};
  }
};

struct WeightedLocation {
  weight_type weight;
  Location loc;

  friend bool operator < (WeightedLocation const& lhs, WeightedLocation const& rhs) {
    return lhs.weight < rhs.weight;
  }
  friend bool operator > (WeightedLocation const& lhs, WeightedLocation const& rhs) {
    return lhs.weight > rhs.weight;
  }
};

struct Node {
  weight_type g = std::numeric_limits<weight_type>::max();
  weight_type h = 0;
  weight_type weight = 0;
  bool closed = false;
  Location parent;
};

struct Map {
  std::size_t rows;
  std::size_t cols;
  std::unique_ptr<Node[]> nodes;

  Map(std::size_t rows_, std::size_t cols_):
    rows{rows_},
    cols{cols_},
    nodes{std::make_unique<Node[]>(rows * cols)}
  {}

  Map(Map const& other):
    rows{other.rows},
    cols{other.cols},
    nodes{std::make_unique<Node[]>(rows * cols)}
  {
    std::copy(&other.nodes[0], &other.nodes[rows * cols], &nodes[0]);
  }
  Map& operator = (Map const& other) {
    rows = other.rows;
    cols = other.cols;
    nodes = std::make_unique<Node[]>(rows * cols);
    std::copy(&other.nodes[0], &other.nodes[rows * cols], &nodes[0]);
    return *this;
  }

  Map(Map&&) = default;
  Map& operator = (Map&&) = default;

  struct Row {
    Node* nodes;

    constexpr Node& operator [] (std::size_t col) noexcept {
      return nodes[col];
    }
    constexpr Node const& operator [] (std::size_t col) const noexcept {
      return nodes[col];
    }
  };

  Row operator [] (std::size_t row) noexcept {
    return {nodes.get() + row * cols};
  }
  Row operator [] (std::size_t row) const noexcept {
    return {nodes.get() + row * cols};
  }
  Node& operator [] (Location const& loc) const noexcept {
    return operator [] (loc.row)[loc.col];
  }
};

template <typename T> constexpr T absdiff(T x, T y) {
#if defined(__GNUC__) || defined(__clang__)
  T result;
  if (__builtin_sub_overflow(x, y, &result))
    return y - x;
  return result;
#else
  return x > y ? x - y : y - x;
#endif
}

constexpr auto mdist(Location start, Location finish) {
  return absdiff(start.row, finish.row) + absdiff(start.col, finish.col);
}

weight_type find_paths(Map, Location const&, Location const&);


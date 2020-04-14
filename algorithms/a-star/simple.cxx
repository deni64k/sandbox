#include <tuple>
#include <set>
#include <queue>
#include <limits>
#include <ostream>
#include <istream>
#include <iostream>

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

using CellType = unsigned;

struct Location {
  CellType row;
  CellType col;

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
  Location loc;
  CellType weight;

  friend bool operator < (WeightedLocation const& lhs, WeightedLocation const& rhs) {
    return lhs.weight < rhs.weight;
  }
  friend bool operator > (WeightedLocation const& lhs, WeightedLocation const& rhs) {
    return lhs.weight > rhs.weight;
  }
};

struct Cell {
  Location parent;
  CellType g = std::numeric_limits<CellType>::max();
  CellType h = 0;
  CellType weight = 0;
};

template <typename T> T abssub(T x, T y) {
  return x > y ? x - y : y - x;
}

auto mdist(Location start, Location finish) {
  return abssub(start.row, finish.row) + abssub(start.col, finish.col);
}

std::vector<Location>
get_neighbors(std::vector<std::vector<Cell>> const& map, Location const& loc) {
  std::vector<Location> neighbors;
  neighbors.reserve(4);
  if (loc.col > 0)
    neighbors.push_back({loc.row, loc.col - 1});
  if (loc.row > 0)
    neighbors.push_back({loc.row - 1, loc.col});
  if (loc.col < map[0].size() - 1)
    neighbors.push_back({loc.row, loc.col + 1});
  if (loc.row < map.size() - 1)
    neighbors.push_back({loc.row + 1, loc.col});
  return neighbors;
}

auto find_paths(std::vector<std::vector<Cell>> map,
                Location const& start, Location const& finish) {
  std::priority_queue<WeightedLocation, std::vector<WeightedLocation>,
                      std::greater<WeightedLocation>> opened;
  std::set<Location> closed;

  auto& cell = map[start.row][start.col];
  cell.g = 0;
  cell.h = mdist(start, finish);
  cell.parent = start;
  opened.push({start, cell.g + cell.h});

  while (!opened.empty()) {
    auto [loc, weight] = opened.top();
    opened.pop();

    if (loc == finish)
      break;

    auto& cell = map[loc.row][loc.col];

    for (auto const& loc_neighbor : get_neighbors(map, loc)) {
      auto& neighbor = map[loc_neighbor.row][loc_neighbor.col];
      CellType cost = cell.g + neighbor.weight;
      if (neighbor.g <= cost)
        continue;

      if (neighbor.weight >= 100)
        continue;

      neighbor.g = cost;
      neighbor.h = mdist(loc_neighbor, finish); 
      neighbor.parent = loc;

      closed.erase(loc_neighbor);

      opened.push({loc_neighbor, neighbor.g + neighbor.h});
    }

    closed.insert(loc);
  }

  return map;
}

int main(int, char**) {
  CellType rows, cols;
  std::cin >> rows >> cols;

  std::vector<std::vector<Cell>> map{rows, std::vector<Cell>{cols}};
  for (CellType r = 0; r < rows; ++r) {
    for (CellType c = 0; c < cols; ++c) {
      auto& cell = map[r][c];
      std::cin >> cell.weight;
      cell.parent = {r, c};
    }
  }

  unsigned q;
  std::cin >> q;

  for (unsigned i = 0; i < q; ++i) {
    Location start, finish;
    std::cin >> start >> finish;

    auto paths = find_paths(map, start, finish);
    auto cost = paths[finish.row][finish.col].g;
    std::cout << "Cost from (" << start << ") to (" << finish << ") is: ";
    if (cost != std::numeric_limits<CellType>::max())
      std::cout << cost;
    else
      std::cout << "unreachable";
    std::cout << '\n';
    std::cout << "Paths:\n";
    for (auto const& row : paths) {
      for (auto const& col : row) {
        std::cout << '\t';
        if (col.g != std::numeric_limits<CellType>::max())
          std::cout << col.g;
        else
          std::cout << '-';
      }
      std::cout << '\n';
    }
    std::cout << "Path: (" << finish << ')';
    for (Location loc = finish;;) {
      auto parent = paths[loc.row][loc.col].parent;
      if (parent == loc) {
        std::cout << " <- (unreachable)";
        break;
      }
      loc = parent;
      std::cout << " <- " << '(' << loc << ')';
      if (loc == start)
        break;
    }
    std::cout << '\n';
    std::cout << '\n';
  }
}

#include <iostream>

#include "astar.hxx"

int main(int, char**) {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);
  
  std::size_t rows, cols;
  std::cin >> rows >> cols;

  auto map = Map(rows, cols);
  for (std::size_t r = 0; r < map.rows; ++r) {
    for (std::size_t c = 0; c < map.cols; ++c) {
      auto& cell = map[r][c];
      std::cin >> cell.weight;
      cell.parent = Location{r, c};
    }
  }

  std::size_t q;
  std::cin >> q;

  for (std::size_t i = 0; i < q; ++i) {
    Location start, finish;
    std::cin >> start >> finish;

    auto cost = find_paths(map, start, finish);
    std::cout << cost << '\n';

#if 0
    auto paths = find_paths(map, start, finish);
    auto cost = paths[finish.row][finish.col].g;
    std::cout << cost << '\n';
    continue;
    
    std::cout << "Cost from (" << start << ") to (" << finish << ") is: ";
    if (cost != std::numeric_limits<weight_type>::max())
      std::cout << cost;
    else
      std::cout << "unreachable";
    std::cout << '\n';
    std::cout << "Paths:\n";
    for (std::size_t r = 0; r < map.rows; ++r) {
      for (std::size_t c = 0; c < map.cols; ++c) {
        auto const& col = paths[r][c];
        std::cout << '\t';
        if (col.g != std::numeric_limits<weight_type>::max())
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
#endif
  }
}

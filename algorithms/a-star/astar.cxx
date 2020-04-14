#include <algorithm>
#include <vector>
#include <deque>
#include <cmath>
#include <iostream>

#include "astar.hxx"

static
auto get_neighbors(Map const& map, Location const& loc) {
  std::array<Location, 4> neighbors;
  std::size_t num_neighbors = 0;
  if (loc.col > 0)
    neighbors[num_neighbors++] = {loc.row, loc.col - 1};
  if (loc.row > 0)
    neighbors[num_neighbors++] = {loc.row - 1, loc.col};
  if (loc.col < map.cols - 1)
    neighbors[num_neighbors++] = {loc.row, loc.col + 1};
  if (loc.row < map.rows - 1)
    neighbors[num_neighbors++] = {loc.row + 1, loc.col};
  return std::tuple{neighbors, num_neighbors};
}

struct HeapInsertor {
  HeapInsertor(std::vector<WeightedLocation>& heap):
    first_insert_{true},
    heap_{heap}
  {
    iter_d_ = std::distance(heap_.begin(), heap_.end());
    std::pop_heap(heap_.begin(), heap_.end(), std::greater<WeightedLocation>());
  }

  ~HeapInsertor() {
    auto iter_ = heap_.begin();
    std::advance(iter_, iter_d_);
    if (first_insert_) {
      heap_.pop_back();
      return;
    }

    do {
      std::push_heap(heap_.begin(), iter_, std::greater<WeightedLocation>{});
      if (iter_ == heap_.end())
        break;
      ++iter_;
    } while (true);
  }

  void push(WeightedLocation&& wl) {
    if (first_insert_) {
      heap_.back() = std::move(wl);
      first_insert_ = false;
    } else {
      heap_.push_back(std::move(wl));
    }
  }

  void decrease(weight_type weight, std::vector<WeightedLocation>::iterator iter) {
    iter->weight = weight;
    std::push_heap(heap_.begin(), iter + 1, std::greater<WeightedLocation>{});
  }

private:
  bool first_insert_;
  std::vector<WeightedLocation>& heap_;
  std::size_t iter_d_;
};

weight_type find_paths(Map map,
                       Location const& start, Location const& finish) {
  auto& cell = map[start.row][start.col];
  cell.g = map[start.row][start.col].weight;
  cell.h = mdist(start, finish);

  std::vector<WeightedLocation> opened = {{cell.g + cell.h, start}};
  // opened.reserve(std::max(map.rows, map.cols));
  // opened.reserve(map.rows * map.cols);

  auto min_cost = std::numeric_limits<weight_type>::max();
  
  while (!opened.empty()) {
    auto& head = opened.front();
    // auto weight = head.weight;

    auto loc = head.loc;
    auto& cell = map[loc.row][loc.col];

    HeapInsertor insertor(opened);

    if (loc == finish) {
      min_cost = std::min(min_cost, cell.g);
      continue;
    }

    if (min_cost < cell.g) {
      continue;
    }

    auto [neighbors, num_neighbors] = get_neighbors(map, loc);
    for (std::size_t i_neighbors = 0; i_neighbors < num_neighbors; ++i_neighbors) {
      auto const& loc_neighbor = neighbors[i_neighbors];
      auto& neighbor = map[loc_neighbor.row][loc_neighbor.col];
      weight_type cost = cell.g + neighbor.weight;
      if (neighbor.closed && neighbor.g <= cost)
        continue;

      // if (neighbor.closed && neighbor.g == cost)
      //   continue;

      if (neighbor.parent == loc)
        continue;

      //if (neighbor.weight >= 100)
      //  continue;

      std::cerr << __func__ << ":" << __LINE__ << ": "
                << "loc_neighbor[" << i_neighbors << "]= " << loc_neighbor
                << "; cost= " << cost << '\n';
      neighbor.g = cost;
      neighbor.closed = false;
      // neighbor.h = mdist(loc_neighbor, finish);
      neighbor.h = neighbor.weight ? mdist(loc_neighbor, finish) : 0;

      auto iter = opened.rbegin();
      for (; iter != opened.rend(); ++iter) {
        if (iter->loc == loc_neighbor) {
          map[loc_neighbor].parent = loc;
          insertor.decrease(neighbor.g + neighbor.h, iter.base() - 1);
          // iter->weight = neighbor.g + neighbor.h;
          break;
        }
      }
      if (iter == opened.rend()) {
        insertor.push({neighbor.g + neighbor.h, loc_neighbor});
        // opened.push_back({loc_neighbor, neighbor.g + neighbor.h});
      }
      // std::push_heap(opened.begin(), iter, std::greater<WeightedLocation>());
    }

    cell.closed = true;
  }

  return min_cost;
}

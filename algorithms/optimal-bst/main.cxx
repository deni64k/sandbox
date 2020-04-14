#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>

void solution(std::vector<double> const &probs) {
  auto const n = probs.size();
  std::vector<std::vector<double>> memo(n,
    std::vector<double>(n, std::numeric_limits<double>::max()));

  for (int i = 0; i < n; ++i) {
    memo[i][i] = probs[i];
  }

  for (int s = 1; s < n; ++s) {
    for (int i = 0; i < n; ++i) {
      auto const j = i + s;
      if (j >= n) {
        i = n;
        continue;
      }

      double alt0 = std::accumulate(probs.cbegin()+i, probs.cbegin()+j+1,
                                    0.0, std::plus<double>());
      for (int r = i; r <= j; ++r) {
        double alt = alt0;
        if (r-1 >= i)
          alt += memo[i][r-1];
        if (r+1 <= j)
          alt += memo[r+1][j];

        memo[i][j] = std::min(memo[i][j], alt);
      }
    }
  }

  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < n; ++j) {
      std::cout << std::setw(16) << memo[i][j] << " ";
    }
    std::cout << '\n';
  }
}

int main(int argc, char *argv[]) {
  std::vector<double> probs = {{0.2, 0.05, 0.17, 0.1, 0.2, 0.03, 0.25}};

  solution(probs);

  return 0;
}

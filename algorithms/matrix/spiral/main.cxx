// cxx = g++-8
// cxxflags = -std=c++17

#include <bits/stdc++.h>

using namespace std;

void solve();

int main(int argc, char **) {
  std::ios::sync_with_stdio(false);
  std::cin.tie(nullptr);

  int Q;
  cin >> Q;

  while (Q--) {
    solve();
  }

  return 0;
}

void print(const vector<vector<int>>& m,
           int top_row, int bottom_row,
           int left_col, int right_col) {
  int r, c;
  for (c = left_col; c <= right_col; ++c)
    cout << m[top_row][c] << ' ' << flush;

  for (r = top_row + 1; r <= bottom_row; ++r)
    cout << m[r][right_col] << ' ' << flush;

  if (bottom_row != top_row)
    for (c = right_col - 1; c >= left_col; --c)
      cout << m[bottom_row][c] << ' ' << flush;

  if (left_col != right_col)
    for (r = bottom_row - 1; r >= top_row + 1; --r)
      cout << m[r][left_col] << ' ' << flush;
}

void solve() {
  int rows, cols;
  cin >> rows >> cols;
  vector<vector<int>> m(rows, vector<int>(cols, 0));
  for (int i = 0; i < rows; ++i)
    for (int j = 0; j < cols; ++j)
      cin >> m[i][j];

  int top_row = 0;
  int bottom_row = rows - 1;
  int left_col = 0;
  int right_col = cols - 1;

  for (; top_row <= bottom_row && left_col <= right_col;
       ++top_row, --bottom_row, ++left_col, --right_col) {
    print(m, top_row, bottom_row, left_col, right_col);
  }
  cout << endl;
}

#include <iostream>
#include <vector>
using namespace std;

struct Rect {
  int top;
  int left;
  int bottom;
  int right;
};

Rect find_rectangle(vector<vector<int>> image) {
  // Left top coordinates
  int x0 = (unsigned)(-1) >> 1; 
  int y0 = (unsigned)(-1) >> 1;
  
  // Right bottom coordinates
  int x1 = 0, y1 = 0;

  for (int i = 0; i < image.size(); ++i) {
    auto const &row = image[i];
    for (int j = 0; j < row.size(); ++j) {
      auto const pixel = row[j];
      
      if (pixel == 0) {
        find_subrectangle(image, j, i);
      }
    }
  }
  
  return Rect{y0, x0, y1, x1};
}

Rect find_subrectangle(vector<vector<int>> &image, int left, int top) {
  // Right bottom coordinates
  int x1 = 0, y1 = 0;

  int width = 0;
  int height = 0;
  
  auto const &row = image[top];
  for (int i = left + 1; i < row.size(); ++i) {
    if (row[i] == 0)
      ++width;
  }

  for (int i = top + 1; i < image.size(); ++i) {
    if (image[i][left] == 0)
      ++height;
  }

  for (int i = left; i < 
    auto const &row = image[i];
    for (int j = right; j < row.size(); ++j) {
      auto const pixel = row[j];
      
      if (pixel == 0) {
        x0 = min(x0, j);
        y0 = min(y0, i);
        x1 = max(x1, j);
        y1 = max(y1, i);
      }
    }
  }
  
  return Rect{y0, x0, y1, x1};
}

// To execute C++, please define "int main()"
int main() {
  vector<vector<int>> image = {
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 0, 0, 0, 1 },
    { 1, 1, 1, 0, 0, 0, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
  };
  vector<vector<int>> image0 = {
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1 },
  };
  
  Rect rect = find_rectangle(image0);
  
  printf("top left (%d, %d)\n", rect.top, rect.left);
  printf("bottom right (%d, %d)\n", rect.bottom, rect.right);
  
  return 0;
}


/*
Imagine we have an image. We’ll represent this image as a simple 2D array where every pixel is a 1 or a 0. 

The image you get is known to have N rectangles of 0s on a background of 1s. Write a function that takes in the image and outputs the coordinates of all the 0 rectangles -- top-left and bottom-right; or top-left, width and height.

//C++
vector<vector<int>> image = {
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
  {1, 1, 1, 0, 0, 0, 1},
  {1, 0, 1, 0, 0, 0, 1},
  {1, 0, 1, 1, 1, 1, 1},
  {1, 0, 1, 0, 0, 1, 1},
  {1, 1, 1, 0, 0, 1, 1},
  {1, 1, 1, 1, 1, 1, 1},
};

0 in col 3, row 2
 */

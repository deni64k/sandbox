#include <bits/stdc++.h>

#if 1
# define DEBUG(x)  {x;}
# define TRACEV(v) {std::cerr << __func__ << ": " << __LINE__ << ": " << #v << v << '\n';}
# define TRACE(v)  {std::cerr << __func__ << ": " << __LINE__ << '\n';}
#else
# define DEBUG(x)
# define TRACEV(v)
# define TRACE(v)
#endif

using namespace std;

void problem();

int main() {
  int T;

  cin >> T;
  for (int i = 1; i <= T; ++i) {
    std::cout << "Case #" << i << ": ";
    problem(i);
  }

  return 0;
}

void problem() {
  std::string cakes;
  int pan;
  cin >> cakes >> pan;

  
}

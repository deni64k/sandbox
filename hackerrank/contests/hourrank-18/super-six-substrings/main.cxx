// https://www.hackerrank.com/contests/hourrank-18/challenges/super-six-substrings

#include <cmath>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>

int main() {
    std::string s;
    std::cin >> s;
    
    if (s.empty()) {
        std::cout << 0 << std::endl;
        return 0;
    }

    char* digs = new char[s.size()];
    char* memo = new char[s.size()];
    digs[0] = s[0] - '0';
    for (int i = 1; i < s.size(); ++i) {
        int d = s[i] - '0';
        digs[i] = d;
        memo[i] = (memo[i-1] + d) % 3;
    }
    
    long answer = 0;
    for (int i = 0; i < s.size(); ++i) {
        if (digs[i] == 0) {
            ++answer;
            continue;
        }

        int pred = 0;
        for (int j = i; j < s.size(); ++j) {
            pred += digs[j];
            if (pred % 3 == 0 && digs[j] % 2 == 0) {
                ++answer;
                pred = 0;
            }
        }
    }
    
    std::cout << answer << std::endl;
    
    delete[] digs;
    delete[] memo;
    
    return 0;
}

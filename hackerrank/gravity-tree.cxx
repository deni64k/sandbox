#pragma GCC optimize("Ofast")
#pragma GCC target("tune=native")

// https://www.hackerrank.com/contests/w23/challenges/gravity-1
// ruby -e 'print "100000\n"; 99999.times { |i| print i+1, " "}; print "\n10000\n"; 10000.times { |i| print ("#{90000+i} 1\n") }' > i-worst.txt

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <iostream>
#include <algorithm>
#include <queue>
#include <stack>
#include <map>
#include <strings.h>
#include <stdio.h>

typedef std::vector<std::vector<int>> Graph;

typedef long int Distance;

struct Dist {
    Dist(int v_, Distance d_)
    : v(v_)
    , d(d_)
    {}

    int v;
    Distance d;
};

inline
bool operator > (Dist const &l, Dist const &r) {
    return l.d > r.d;
}

inline
Distance calc_attractiveness(Graph const &graph, int const s) {
    bool seen[graph.size()];
    bzero(seen, sizeof(seen));
    seen[s] = true;

    std::stack<Dist> queue;
    queue.emplace(s, 0);

    Distance answer = 0;
    while (!queue.empty()) {
        Dist next = queue.top();
        queue.pop();

        auto const u = next.v;
        auto const d = next.d;

        auto const &ns = graph[u];
        for (auto iter = ns.cbegin(); iter != ns.cend(); ++iter) {
            auto const v = *iter;

            if (seen[v]) {
                continue;
            }
            queue.emplace(v, d+1);
            seen[v] = true;
        }
        answer += d*d;
    }
    
    return answer;
}

inline
Distance calc_attractiveness_height(Graph const &graph, int const s,
                                    int const height) {
    bool seen[graph.size()];
    bzero(seen, sizeof(seen));
    seen[s] = true;

    std::stack<Dist> queue;
    queue.emplace(s, 0);

    Distance answer = 0;
    while (!queue.empty()) {
        Dist next = queue.top();
        queue.pop();

        auto const u = next.v;
        auto const d = next.d;

        auto const &ns = graph[u];
        for (auto iter = ns.cbegin(); iter != ns.cend(); ++iter) {
            auto const v = *iter;

            if (seen[v]) {
                continue;
            }
            queue.emplace(v, d+1);
            seen[v] = true;
        }
        auto const x = d+height;
        answer += x*x;
    }
    
    return answer;
}

inline
Distance calc_attractiveness_barrier(Graph const &graph, int const s,
                                     int const barrier) {
    bool seen[graph.size()];
    bzero(seen, sizeof(seen));
    seen[s] = true;

    std::stack<Dist> queue;
    queue.emplace(s, 0);

    Distance answer = 0;
    while (!queue.empty()) {
        Dist next = queue.top();
        queue.pop();

        auto const u = next.v;
        auto const d = next.d;

        auto const &ns = graph[u];
        for (auto iter = ns.cbegin(); iter != ns.cend(); ++iter) {
            auto const v = *iter;
            if (v == barrier)
                continue;

            if (seen[v]) {
                continue;
            }
            queue.emplace(v, d+1);
            seen[v] = true;
        }
        answer += d*d;
    }
    
    return answer;
}

inline
std::vector<Distance> calc_levels(Graph const &graph) {
    std::vector<Distance> dists(graph.size(), 0);

    std::queue<int> queue;
    queue.emplace(0);

    while (!queue.empty()) {
        int u = queue.front();
        queue.pop();

        auto const &ns = graph[u];
        for (auto iter = ns.cbegin(); iter != ns.cend(); ++iter) {
            auto const v = *iter;

            dists[v] = dists[u] + 1;
            queue.emplace(v);
        }
    }
    
    return std::move(dists);
}

inline
std::vector<int> calc_backjumps(Graph const &graph, std::vector<int> const &parents) {
    std::vector<int> backjumps(parents);
    
    std::stack<std::pair<int, int>> queue;
    queue.emplace(0, 0);

    while (!queue.empty()) {
        int u, p;
        std::tie(u, p) = queue.top();
        queue.pop();

        backjumps[u] = p;
        if (graph[u].size() > 1)
            p = u;

        auto const &ns = graph[u];
        for (auto iter = ns.cbegin(); iter != ns.cend(); ++iter) {
            auto const v = *iter;

            queue.emplace(v, p);
        }
    }
    
    return std::move(backjumps);
}

/*
std::pair<int, int>
find_common_siblings(std::vector<int> const &parents,
                     std::vector<Distance> const &levels,
                     int given_v,
                     int turned_v) {
    while (levels[given_v] != levels[turned_v]) {
        if (levels[given_v] < levels[turned_v]) {
            turned_v = parents[turned_v];
        } else if (levels[given_v] > levels[turned_v]) {
            given_v = parents[given_v];
        } else {
            turned_v = parents[turned_v];
            given_v = parents[given_v];
        }
    }
    return std::make_pair(given_v, turned_v);
}
*/

int
find_common_ancestor(std::vector<int> const &parents,
                     std::vector<int> const &backjumps,
                     std::vector<Distance> const &levels,
                     int given_v,
                     int turned_v) {
    while (given_v != turned_v) {
        if (levels[given_v] < levels[turned_v]) {
            turned_v = parents[turned_v];
        } else if (levels[given_v] > levels[turned_v]) {
            given_v = parents[given_v];
        } else {
            turned_v = backjumps[turned_v];
            given_v = backjumps[given_v];
        }
    }
    return given_v;
}

// v is a turned on vertex
// u is a vertex to measure attractivness of the v subtree.
Distance solution(Graph const &graph,
                  Graph const &undirected_graph,
                  std::vector<int> const &parents,
                  std::vector<int> const &backjumps,
                  std::vector<Distance> const &levels,
                  int const given_v,
                  int const turned_v) {
    // fprintf(stderr, "given_v=%d turned_v=%d\n", given_v+1, turned_v+1);
    
    if (turned_v == given_v) {
        // fprintf(stderr, "same case\n");
        return calc_attractiveness(graph, given_v);
    }

    // int sibling_given, sibling_turned;
    // std::tie(sibling_given, sibling_turned) = find_common_siblings(parents, levels, given_v, turned_v);
    // fprintf(stderr, "sibling_given=%d sibling_turned=%d\n", sibling_given+1, sibling_turned+1);
    int const common_ancestor = find_common_ancestor(parents, backjumps, levels, given_v, turned_v);
    // fprintf(stderr, "common_ancestor=%d\n", common_ancestor+1);

    // if (sibling_given != turned_v && levels[sibling_given] == levels[sibling_turned]) {
    if (common_ancestor != turned_v) {
        // fprintf(stderr, "outside case\n");
        // int const common_ancestor = find_common_ancestor(parents, backjumps, levels, sibling_given, sibling_turned);
        // fprintf(stderr, "common_ancestor=%d\n", common_ancestor+1);
        // The given vertex is outside of the turned v subtree. All we need is
        // to sum the common ancestor heights with the turned on nodes.
        Distance height;
        height = levels[given_v] - levels[common_ancestor];
        height += levels[turned_v] - levels[common_ancestor];
        // fprintf(stderr, "height=%ld\n", height);

        // static std::vector<Distance> memo(graph.size(), -1);
        // static int last_height = height;
        // if (last_height != height) {
        //     std::fill(memo.begin(), memo.end(), -1);
        // }
        // auto &m = memo[turned_v];
        // if (m < 0) {
        //     m = calc_attractiveness_height(graph, turned_v, height);
        // }
        // return m;
        return calc_attractiveness_height(graph, turned_v, height);
    }
    // fprintf(stderr, "inside case\n");
    // The given vertex is inside of the turned v subtree.
    // Now, we need to run DFS and prevent it from handling outside of the turned vertex.
    if (turned_v == 0) {
        return calc_attractiveness(undirected_graph, given_v);
    } else {
        return calc_attractiveness_barrier(undirected_graph, given_v, parents[turned_v]);
    }
}

static int read_int() {
    int c, n;

    // Skip junk.
    while (!::isdigit((c = getchar_unlocked()))) ;

    // The last character read is a digit.
    n = c - '0';

    // Read an integer.
    while (::isdigit((c = getchar_unlocked())))
        n = 10*n + c-'0';

    return n;
}

static void print_long(long int n) {
    char s[20];
    snprintf(s, sizeof(s), "%ld\n", n);
    for (char *c = s; *c != '\n'; ++c) {
        putchar_unlocked(*c);
    }
    putchar_unlocked('\n');
}

int main() {
    std::ios::sync_with_stdio(false);

    int n = read_int();

    Graph graph(n);
    std::vector<int> parents(n);
    parents[0] = 0;
    for (int i = 1; i < n; ++i) {
        int p = read_int();
        // ::scanf("%d", &p);
        --p;
        if (p == i)
            continue;
        graph[p].push_back(i);
        parents[i] = p;
    }

    auto const levels = calc_levels(graph);
    auto const backjumps = calc_backjumps(graph, parents);
    /*
    std::cerr << "dists=[";
    for (auto const l : graph_levels) {
        std::cerr << l << ", ";
    }
    std::cerr << "]" << std::endl;
    */

    Graph undirected_graph(graph.size());
    for (int i = 0; i < graph.size(); ++i) {
        auto const &neighbours = graph[i];
        for (auto iter = neighbours.cbegin(); iter != neighbours.cend(); ++iter) {
            undirected_graph[i].push_back(*iter);
        }
        undirected_graph[i].push_back(parents[i]);
    }

    int q = read_int();
    for (int i = 0; i < q; ++i) {
        int u, v;
        u = read_int();
        v = read_int();
        
        --u;
        --v;

        auto const answer = solution(graph,
                                     undirected_graph,
                                     parents,
                                     backjumps,
                                     levels,
                                     u,
                                     v);

        print_long(answer);
    }
    
    return 0;
}

#include "MazeGenerator.h"
#include <stack>
#include <algorithm>

namespace {
struct Neighbor { Coord pos; Coord from; };

std::vector<Coord> neighbors4(const MazeGrid& grid, const Coord& c) {
    static const int dx[4] = {0, 1, 0, -1};
    static const int dy[4] = {-1, 0, 1, 0};
    std::vector<Coord> res;
    for (int i = 0; i < 4; ++i) {
        int nx = c.x + dx[i];
        int ny = c.y + dy[i];
        if (grid.inBounds(nx, ny)) res.push_back({nx, ny});
    }
    return res;
}
}

void MazeGenerator::generate(MazeGrid& grid, const MazeConfig& cfg) {
    // Reset grid: all walls up, visited false
    for (int y = 0; y < grid.height(); ++y) for (int x = 0; x < grid.width(); ++x) {
        auto &cell = grid.at(x,y);
        cell.visited = false;
        cell.wallN = cell.wallE = cell.wallS = cell.wallW = true;
    }

    std::random_device rd;
    uint64_t s = cfg.seed == 0 ? ((static_cast<uint64_t>(rd()) << 32) ^ rd()) : cfg.seed;
    std::mt19937_64 rng(s);

    switch (cfg.algorithm) {
        case MazeAlgorithm::RecursiveBacktracking:
            genRecursiveBacktracking(grid, rng);
            break;
        case MazeAlgorithm::Prims:
            genPrims(grid, rng);
            break;
    }
}

void MazeGenerator::genRecursiveBacktracking(MazeGrid& grid, std::mt19937_64& rng) {
    std::uniform_int_distribution<int> distX(0, grid.width()-1);
    std::uniform_int_distribution<int> distY(0, grid.height()-1);
    Coord start{distX(rng), distY(rng)};

    std::stack<Coord> st;
    st.push(start);
    grid.at(start.x, start.y).visited = true;

    while (!st.empty()) {
        Coord current = st.top();
        auto neigh = neighbors4(grid, current);
        std::shuffle(neigh.begin(), neigh.end(), rng);

        bool carved = false;
        for (const auto &n : neigh) {
            if (!grid.at(n.x, n.y).visited) {
                grid.removeWallBetween(current, n);
                grid.at(n.x, n.y).visited = true;
                st.push(n);
                carved = true;
                break;
            }
        }
        if (!carved) st.pop();
    }
}

void MazeGenerator::genPrims(MazeGrid& grid, std::mt19937_64& rng) {
    std::uniform_int_distribution<int> distX(0, grid.width()-1);
    std::uniform_int_distribution<int> distY(0, grid.height()-1);
    Coord start{distX(rng), distY(rng)};

    std::vector<Neighbor> frontier;
    grid.at(start.x, start.y).visited = true;
    for (auto &n : neighbors4(grid, start)) frontier.push_back({n, start});

    while (!frontier.empty()) {
        std::uniform_int_distribution<size_t> pick(0, frontier.size()-1);
        size_t idx = pick(rng);
        Neighbor f = frontier[idx];
        frontier.erase(frontier.begin() + idx);

        if (grid.at(f.pos.x, f.pos.y).visited) continue;
        grid.removeWallBetween(f.from, f.pos);
        grid.at(f.pos.x, f.pos.y).visited = true;
        for (auto &n : neighbors4(grid, f.pos)) {
            if (!grid.at(n.x, n.y).visited) frontier.push_back({n, f.pos});
        }
    }
}

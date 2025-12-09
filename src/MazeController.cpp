#include "MazeController.h"
#include <algorithm>
#include <random>

static std::vector<Coord> neighbors4(const MazeGrid& grid, const Coord& c) {
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

void MazeController::startPreview(const MazeConfig& cfg) {
    cfg_ = cfg;
    previewActive_ = true;
    initPreview();
}

void MazeController::initPreview() {
    for (int y = 0; y < grid_->height(); ++y) for (int x = 0; x < grid_->width(); ++x) {
        auto &cell = grid_->at(x,y);
        cell.visited = false;
        cell.wallN = cell.wallE = cell.wallS = cell.wallW = true;
    }
    visitedCount_ = 0;
    stack_.clear();

    std::random_device rd;
    uint64_t s = cfg_.seed == 0 ? ((static_cast<uint64_t>(rd()) << 32) ^ rd()) : cfg_.seed;
    std::mt19937_64 rng(s);
    std::uniform_int_distribution<int> distX(0, grid_->width()-1);
    std::uniform_int_distribution<int> distY(0, grid_->height()-1);
    Coord start{distX(rng), distY(rng)};
    stack_.push_back(start);
    grid_->at(start.x, start.y).visited = true;
    visitedCount_ = 1;
}

bool MazeController::stepPreview() {
    if (!previewActive_) return false;
    if (stack_.empty()) { previewActive_ = false; return false; }

    Coord current = stack_.back();
    auto neigh = neighbors4(*grid_, current);
    std::vector<Coord> unvis;
    for (auto &n : neigh) if (!grid_->at(n.x,n.y).visited) unvis.push_back(n);
    if (!unvis.empty()) {
        Coord n = unvis.back();
        grid_->removeWallBetween(current, n);
        grid_->at(n.x,n.y).visited = true;
        stack_.push_back(n);
        visitedCount_++;
        return true;
    } else {
        stack_.pop_back();
        return !stack_.empty();
    }
}

float MazeController::previewProgress() const {
    int total = grid_->width() * grid_->height();
    return total > 0 ? float(visitedCount_) / float(total) : 0.0f;
}

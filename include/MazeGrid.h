#pragma once
#include "MazeTypes.h"
#include <vector>
#include <stdexcept>

class MazeGrid {
public:
    MazeGrid(int w, int h) : width_(w), height_(h), cells_(w * h) {
        if (w <= 0 || h <= 0) throw std::invalid_argument("MazeGrid dimensions must be positive");
    }

    int width() const { return width_; }
    int height() const { return height_; }

    Cell& at(int x, int y) { return cells_[index(x,y)]; }
    const Cell& at(int x, int y) const { return cells_[index(x,y)]; }

    bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }

    void resetVisited() {
        for (auto &c : cells_) c.visited = false;
    }

    // Utility to remove wall between two adjacent cells
    void removeWallBetween(const Coord& a, const Coord& b) {
        int dx = b.x - a.x;
        int dy = b.y - a.y;
        if (dx == 1 && dy == 0) { // b is east
            at(a.x,a.y).wallE = false;
            at(b.x,b.y).wallW = false;
        } else if (dx == -1 && dy == 0) { // b is west
            at(a.x,a.y).wallW = false;
            at(b.x,b.y).wallE = false;
        } else if (dx == 0 && dy == 1) { // b is south
            at(a.x,a.y).wallS = false;
            at(b.x,b.y).wallN = false;
        } else if (dx == 0 && dy == -1) { // b is north
            at(a.x,a.y).wallN = false;
            at(b.x,b.y).wallS = false;
        }
    }

private:
    int index(int x, int y) const { return y * width_ + x; }
    int width_;
    int height_;
    std::vector<Cell> cells_;
};

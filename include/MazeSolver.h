#pragma once
#include "MazeGrid.h"
#include <vector>
#include <optional>

struct MazePath {
    std::vector<Coord> nodes; // sequence from start to goal
};

class MazeSolver {
public:
    static std::optional<MazePath> solveBFS(const MazeGrid& grid, Coord start, Coord goal);
};

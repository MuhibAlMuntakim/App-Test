#pragma once
#include "MazeGrid.h"
#include "MazeTypes.h"
#include <random>

class MazeGenerator {
public:
    static void generate(MazeGrid& grid, const MazeConfig& cfg);

private:
    static void genRecursiveBacktracking(MazeGrid& grid, std::mt19937_64& rng);
    static void genPrims(MazeGrid& grid, std::mt19937_64& rng);
};

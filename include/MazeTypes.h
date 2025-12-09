#pragma once
#include <vector>
#include <cstdint>
#include <random>
#include <optional>

enum class MazeAlgorithm {
    RecursiveBacktracking,
    Prims
};

struct MazeConfig {
    int width = 20;   // columns
    int height = 20;  // rows
    uint64_t seed = 0; // 0 -> random_device
    MazeAlgorithm algorithm = MazeAlgorithm::RecursiveBacktracking;
};

struct Cell {
    bool visited = false;
    // Walls: N E S W
    bool wallN = true;
    bool wallE = true;
    bool wallS = true;
    bool wallW = true;
};

struct Coord { int x; int y; };

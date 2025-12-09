#pragma once
#include "MazeGrid.h"
#include "MazeGenerator.h"
#include "MazeTypes.h"
#include <memory>
#include <vector>

class MazeController {
public:
    explicit MazeController(int w, int h) : grid_(std::make_unique<MazeGrid>(w,h)) {}

    void regenerate(const MazeConfig& cfg) { MazeGenerator::generate(*grid_, cfg); previewActive_ = false; }
    const MazeGrid& grid() const { return *grid_; }

    // Apply walls from an external grid of matching dimensions
    bool applyFromGrid(const MazeGrid& src) {
        if (src.width() != grid_->width() || src.height() != grid_->height()) return false;
        for (int y=0; y<src.height(); ++y){
            for (int x=0; x<src.width(); ++x){
                const Cell &s = src.at(x,y);
                Cell &d = grid_->at(x,y);
                d.wallN = s.wallN; d.wallE = s.wallE; d.wallS = s.wallS; d.wallW = s.wallW; d.visited = true;
            }
        }
        return true;
    }

    // Incremental preview (DFS)
    void startPreview(const MazeConfig& cfg);
    bool stepPreview();
    float previewProgress() const;
    bool previewActive() const { return previewActive_; }

private:
    std::unique_ptr<MazeGrid> grid_;
    MazeConfig cfg_{};
    bool previewActive_ = false;
    std::vector<Coord> stack_;
    int visitedCount_ = 0;
    void initPreview();
};

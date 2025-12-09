#include "MazeGrid.h"
#include "MazeGenerator.h"
#include "MazeIO.h"
#include <QString>
#include <iostream>
#include <memory>

int main(){
    MazeConfig cfg; cfg.width=15; cfg.height=10; cfg.seed=42; cfg.algorithm=MazeAlgorithm::Prims;
    MazeGrid grid(cfg.width,cfg.height);
    MazeGenerator::generate(grid, cfg);
    QString path = "/mnt/data/maze3d/build/test_maze.json";
    if (!MazeIO::saveToJson(grid, cfg, path)) { std::cerr << "Save failed" << std::endl; return 1; }
    std::unique_ptr<MazeGrid> loaded; MazeConfig cfg2{};
    if (!MazeIO::loadCreate(loaded, cfg2, path)) { std::cerr << "Load failed" << std::endl; return 1; }
    if (loaded->width() != grid.width() || loaded->height() != grid.height()) { std::cerr << "Dims mismatch" << std::endl; return 1; }
    for (int y=0; y<grid.height(); ++y){
        for (int x=0; x<grid.width(); ++x){
            const Cell &a = grid.at(x,y); const Cell &b = loaded->at(x,y);
            if (a.wallN != b.wallN || a.wallE != b.wallE || a.wallS != b.wallS || a.wallW != b.wallW){
                std::cerr << "Wall mismatch at " << x << "," << y << std::endl; return 1;
            }
        }
    }
    std::cout << "Persistence roundtrip OK" << std::endl;
    return 0;
}

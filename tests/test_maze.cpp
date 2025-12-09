#include "MazeGrid.h"
#include "MazeGenerator.h"
#include <iostream>
#include <queue>

// Check connectivity: BFS to count reachable cells
int reachableCount(const MazeGrid& grid, int sx, int sy) {
    std::vector<bool> vis(grid.width()*grid.height(), false);
    auto idx = [&](int x,int y){return y*grid.width()+x;};
    std::queue<Coord> q;
    q.push({sx,sy});
    vis[idx(sx,sy)] = true;

    while(!q.empty()){
        Coord c = q.front(); q.pop();
        const Cell &cell = grid.at(c.x,c.y);
        // N
        if(!cell.wallN){ int nx=c.x, ny=c.y-1; if(ny>=0 && !vis[idx(nx,ny)]){ vis[idx(nx,ny)]=true; q.push({nx,ny}); } }
        // E
        if(!cell.wallE){ int nx=c.x+1, ny=c.y; if(nx<grid.width() && !vis[idx(nx,ny)]){ vis[idx(nx,ny)]=true; q.push({nx,ny}); } }
        // S
        if(!cell.wallS){ int nx=c.x, ny=c.y+1; if(ny<grid.height() && !vis[idx(nx,ny)]){ vis[idx(nx,ny)]=true; q.push({nx,ny}); } }
        // W
        if(!cell.wallW){ int nx=c.x-1, ny=c.y; if(nx>=0 && !vis[idx(nx,ny)]){ vis[idx(nx,ny)]=true; q.push({nx,ny}); } }
    }
    int count=0; for(bool b: vis) if(b) ++count; return count;
}

int main(){
    MazeConfig cfg; cfg.width=30; cfg.height=20; cfg.seed=12345; cfg.algorithm=MazeAlgorithm::RecursiveBacktracking;
    MazeGrid grid(cfg.width,cfg.height);
    MazeGenerator::generate(grid, cfg);
    int rc = reachableCount(grid, 0, 0);
    if(rc != cfg.width*cfg.height){
        std::cerr << "Connectivity failed for DFS: " << rc << " / " << (cfg.width*cfg.height) << std::endl;
        return 1;
    }
    cfg.algorithm=MazeAlgorithm::Prims;
    MazeGenerator::generate(grid, cfg);
    rc = reachableCount(grid, 0, 0);
    if(rc != cfg.width*cfg.height){
        std::cerr << "Connectivity failed for Prim's: " << rc << " / " << (cfg.width*cfg.height) << std::endl;
        return 1;
    }
    std::cout << "All tests passed" << std::endl;
    return 0;
}

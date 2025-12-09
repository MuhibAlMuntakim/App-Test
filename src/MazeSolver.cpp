#include "MazeSolver.h"
#include <queue>
#include <algorithm>

static inline int idx(const MazeGrid& g, int x, int y){ return y * g.width() + x; }

std::optional<MazePath> MazeSolver::solveBFS(const MazeGrid& grid, Coord start, Coord goal){
    int W = grid.width(), H = grid.height();
    std::vector<char> vis(W*H, 0);
    std::vector<int> parent(W*H, -1);
    std::queue<Coord> q;
    q.push(start);
    vis[idx(grid,start.x,start.y)] = 1;

    auto tryPush = [&](int x,int y,int fromIndex){
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        int i = idx(grid,x,y);
        if (!vis[i]){ vis[i]=1; parent[i]=fromIndex; q.push({x,y}); }
    };

    while(!q.empty()){
        Coord c = q.front(); q.pop();
        if (c.x == goal.x && c.y == goal.y){
            MazePath path; path.nodes.clear();
            int i = idx(grid,c.x,c.y);
            while(i != -1){
                int y = i / W; int x = i % W;
                path.nodes.push_back({x,y});
                i = parent[i];
            }
            std::reverse(path.nodes.begin(), path.nodes.end());
            return path;
        }
        const Cell& cell = grid.at(c.x,c.y);
        int ci = idx(grid,c.x,c.y);
        if (!cell.wallN) tryPush(c.x, c.y-1, ci);
        if (!cell.wallE) tryPush(c.x+1, c.y, ci);
        if (!cell.wallS) tryPush(c.x, c.y+1, ci);
        if (!cell.wallW) tryPush(c.x-1, c.y, ci);
    }
    return std::nullopt;
}

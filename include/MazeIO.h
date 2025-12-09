#pragma once
#include "MazeGrid.h"
#include "MazeTypes.h"
#include <QString>
#include <QImage>
#include <memory>

namespace MazeIO {
    bool saveToJson(const MazeGrid& grid, const MazeConfig& cfg, const QString& filePath);
    bool loadFromJson(MazeGrid& grid, MazeConfig& cfg, const QString& filePath);
    bool loadCreate(std::unique_ptr<MazeGrid>& outGrid, MazeConfig& outCfg, const QString& filePath);
    bool saveSnapshotPNG(const QImage& image, const QString& filePath);
}

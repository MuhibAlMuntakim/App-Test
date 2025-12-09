#include "MazeIO.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace MazeIO {

static bool parseJsonToGrid(std::unique_ptr<MazeGrid>& outGrid, MazeConfig& outCfg, const QJsonObject& root){
    int w = root.value("width").toInt(0);
    int h = root.value("height").toInt(0);
    if (w <= 0 || h <= 0) return false;
    outCfg.seed = static_cast<uint64_t>(root.value("seed").toDouble(0.0));
    QString alg = root.value("algorithm").toString();
    outCfg.algorithm = (alg == "Prims") ? MazeAlgorithm::Prims : MazeAlgorithm::RecursiveBacktracking;

    QJsonArray cells = root.value("cells").toArray();
    if (cells.size() != w*h) return false;

    outGrid = std::make_unique<MazeGrid>(w,h);
    int i=0;
    for (int y=0; y<h; ++y){
        for (int x=0; x<w; ++x){
            QJsonObject co = cells[i++].toObject();
            Cell &c = outGrid->at(x,y);
            c.wallN = co.value("wallN").toBool(true);
            c.wallE = co.value("wallE").toBool(true);
            c.wallS = co.value("wallS").toBool(true);
            c.wallW = co.value("wallW").toBool(true);
            c.visited = true;
        }
    }
    return true;
}

bool saveToJson(const MazeGrid& grid, const MazeConfig& cfg, const QString& filePath){
    QJsonObject root;
    root["width"] = grid.width();
    root["height"] = grid.height();
    root["seed"] = static_cast<double>(cfg.seed);
    root["algorithm"] = (cfg.algorithm == MazeAlgorithm::RecursiveBacktracking) ? "RecursiveBacktracking" : "Prims";

    QJsonArray cells;
    for (int y=0; y<grid.height(); ++y){
        for (int x=0; x<grid.width(); ++x){
            const Cell& c = grid.at(x,y);
            QJsonObject co;
            co["wallN"] = c.wallN;
            co["wallE"] = c.wallE;
            co["wallS"] = c.wallS;
            co["wallW"] = c.wallW;
            cells.append(co);
        }
    }
    root["cells"] = cells;

    QJsonDocument doc(root);
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool loadFromJson(MazeGrid& grid, MazeConfig& cfg, const QString& filePath){
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.readAll();
    f.close();
    QJsonParseError err; QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;
    QJsonObject root = doc.object();

    std::unique_ptr<MazeGrid> tmp;
    MazeConfig tmpCfg{};
    if (!parseJsonToGrid(tmp, tmpCfg, root)) return false;
    if (tmp->width() != grid.width() || tmp->height() != grid.height()) return false;

    cfg = tmpCfg;
    for (int y=0; y<grid.height(); ++y){
        for (int x=0; x<grid.width(); ++x){
            const Cell &src = tmp->at(x,y);
            Cell &dst = grid.at(x,y);
            dst.wallN = src.wallN; dst.wallE = src.wallE; dst.wallS = src.wallS; dst.wallW = src.wallW; dst.visited = true;
        }
    }
    return true;
}

bool loadCreate(std::unique_ptr<MazeGrid>& outGrid, MazeConfig& outCfg, const QString& filePath){
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.readAll();
    f.close();
    QJsonParseError err; QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;
    QJsonObject root = doc.object();
    return parseJsonToGrid(outGrid, outCfg, root);
}

bool saveSnapshotPNG(const QImage& image, const QString& filePath){
    QImage img = image;
    if (img.isNull()) return false;
    return img.save(filePath, "PNG");
}

} // namespace MazeIO

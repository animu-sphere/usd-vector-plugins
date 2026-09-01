#pragma once

#include "usdvector/core/geometry.h"

#include <optional>
#include <vector>

namespace usdvector {

struct Feature;

struct Bounds {
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    std::optional<double> minZ;
    std::optional<double> maxZ;
    bool empty = true;

    void Include(const Coordinate& coordinate);
    Coordinate Center() const;
};

Bounds ComputeBounds(const Geometry& geometry);
Bounds ComputeBounds(const std::vector<Feature>& features);

}  // namespace usdvector
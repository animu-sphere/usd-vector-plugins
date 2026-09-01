#pragma once

#include "usdvector/core/diagnostics.h"
#include "usdvector/core/geometry.h"

#include <cstdint>
#include <vector>

namespace usdvector::authoring {

struct Triangle {
    std::uint32_t a = 0;
    std::uint32_t b = 0;
    std::uint32_t c = 0;
};

struct PolygonMesh {
    std::vector<Coordinate> vertices;
    std::vector<Triangle> triangles;
};

Result<PolygonMesh> TriangulatePolygon(const Polygon& polygon);

}  // namespace usdvector::authoring

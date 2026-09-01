#include "usdvector/core/bounds.h"
#include "usdvector/core/model.h"

#include <algorithm>
#include <type_traits>

namespace usdvector {
namespace {

void IncludeRing(Bounds& bounds, const Ring& ring) {
    for (const Coordinate& coordinate : ring) {
        bounds.Include(coordinate);
    }
}

void IncludeGeometry(Bounds& bounds, const Geometry& geometry) {
    std::visit(
        [&bounds](const auto& value) {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, Point>) {
                bounds.Include(value.coordinate);
            } else if constexpr (std::is_same_v<Value, MultiPoint>) {
                for (const Coordinate& coordinate : value.coordinates) {
                    bounds.Include(coordinate);
                }
            } else if constexpr (std::is_same_v<Value, LineString>) {
                for (const Coordinate& coordinate : value.coordinates) {
                    bounds.Include(coordinate);
                }
            } else if constexpr (std::is_same_v<Value, MultiLineString>) {
                for (const LineString& line : value.lines) {
                    for (const Coordinate& coordinate : line.coordinates) {
                        bounds.Include(coordinate);
                    }
                }
            } else if constexpr (std::is_same_v<Value, Polygon>) {
                IncludeRing(bounds, value.outer);
                for (const Ring& hole : value.holes) {
                    IncludeRing(bounds, hole);
                }
            } else if constexpr (std::is_same_v<Value, MultiPolygon>) {
                for (const Polygon& polygon : value.polygons) {
                    IncludeRing(bounds, polygon.outer);
                    for (const Ring& hole : polygon.holes) {
                        IncludeRing(bounds, hole);
                    }
                }
            }
        },
        geometry);
}

}  // namespace

void Bounds::Include(const Coordinate& coordinate) {
    if (empty) {
        minX = maxX = coordinate.x;
        minY = maxY = coordinate.y;
        empty = false;
    } else {
        minX = std::min(minX, coordinate.x);
        minY = std::min(minY, coordinate.y);
        maxX = std::max(maxX, coordinate.x);
        maxY = std::max(maxY, coordinate.y);
    }

    if (coordinate.z.has_value()) {
        if (!minZ.has_value()) {
            minZ = maxZ = coordinate.z;
        } else {
            minZ = std::min(*minZ, *coordinate.z);
            maxZ = std::max(*maxZ, *coordinate.z);
        }
    }
}

Coordinate Bounds::Center() const {
    if (empty) {
        return {};
    }
    return {(minX + maxX) / 2.0, (minY + maxY) / 2.0,
            minZ.has_value() ? std::optional<double>{(*minZ + *maxZ) / 2.0}
                              : std::nullopt};
}

Bounds ComputeBounds(const Geometry& geometry) {
    Bounds bounds;
    IncludeGeometry(bounds, geometry);
    return bounds;
}

Bounds ComputeBounds(const std::vector<Feature>& features) {
    Bounds bounds;
    for (const Feature& feature : features) {
        IncludeGeometry(bounds, feature.geometry);
    }
    return bounds;
}

}  // namespace usdvector
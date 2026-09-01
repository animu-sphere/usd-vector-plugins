#include "usdvector/core/geometry.h"

#include <type_traits>

namespace usdvector {

bool operator==(const Coordinate& left, const Coordinate& right) {
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

GeometryType GetGeometryType(const Geometry& geometry) {
    return std::visit(
        [](const auto& value) -> GeometryType {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::monostate>) {
                return GeometryType::Null;
            } else if constexpr (std::is_same_v<Value, Point>) {
                return GeometryType::Point;
            } else if constexpr (std::is_same_v<Value, MultiPoint>) {
                return GeometryType::MultiPoint;
            } else if constexpr (std::is_same_v<Value, LineString>) {
                return GeometryType::LineString;
            } else if constexpr (std::is_same_v<Value, MultiLineString>) {
                return GeometryType::MultiLineString;
            } else if constexpr (std::is_same_v<Value, Polygon>) {
                return GeometryType::Polygon;
            } else {
                return GeometryType::MultiPolygon;
            }
        },
        geometry);
}

const char* GeometryTypeName(GeometryType type) {
    switch (type) {
        case GeometryType::Null:
            return "Null";
        case GeometryType::Point:
            return "Point";
        case GeometryType::MultiPoint:
            return "MultiPoint";
        case GeometryType::LineString:
            return "LineString";
        case GeometryType::MultiLineString:
            return "MultiLineString";
        case GeometryType::Polygon:
            return "Polygon";
        case GeometryType::MultiPolygon:
            return "MultiPolygon";
    }
    return "Unknown";
}

}  // namespace usdvector
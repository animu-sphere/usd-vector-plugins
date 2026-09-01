#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace usdvector {

struct Coordinate {
    double x = 0.0;
    double y = 0.0;
    std::optional<double> z;
};

bool operator==(const Coordinate& left, const Coordinate& right);

using Ring = std::vector<Coordinate>;

struct Point {
    Coordinate coordinate;
};

struct MultiPoint {
    std::vector<Coordinate> coordinates;
};

struct LineString {
    std::vector<Coordinate> coordinates;
};

struct MultiLineString {
    std::vector<LineString> lines;
};

struct Polygon {
    Ring outer;
    std::vector<Ring> holes;
};

struct MultiPolygon {
    std::vector<Polygon> polygons;
};

enum class GeometryType {
    Null,
    Point,
    MultiPoint,
    LineString,
    MultiLineString,
    Polygon,
    MultiPolygon,
};

using Geometry = std::variant<std::monostate, Point, MultiPoint, LineString,
                              MultiLineString, Polygon, MultiPolygon>;

GeometryType GetGeometryType(const Geometry& geometry);
const char* GeometryTypeName(GeometryType type);

}  // namespace usdvector
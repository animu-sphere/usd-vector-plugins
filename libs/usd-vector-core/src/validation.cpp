#include "usdvector/core/validation.h"

#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace usdvector {
namespace {

Diagnostic MakeDiagnostic(DiagnosticCode code, std::string message,
                          std::optional<std::uint32_t> part = std::nullopt,
                          std::optional<std::uint32_t> ring = std::nullopt,
                          std::optional<std::uint64_t> coordinate = std::nullopt) {
    return {code, Severity::Error, std::move(message), std::nullopt,
            std::nullopt, std::nullopt, part, ring, coordinate, std::nullopt};
}

bool IsFinite(const Coordinate& coordinate) {
    return std::isfinite(coordinate.x) && std::isfinite(coordinate.y) &&
           (!coordinate.z.has_value() || std::isfinite(*coordinate.z));
}

std::size_t DistinctCoordinateCount(const std::vector<Coordinate>& coordinates) {
    std::vector<Coordinate> distinct;
    for (const Coordinate& coordinate : coordinates) {
        bool alreadySeen = false;
        for (const Coordinate& candidate : distinct) {
            if (coordinate == candidate) {
                alreadySeen = true;
                break;
            }
        }
        if (!alreadySeen) {
            distinct.push_back(coordinate);
        }
    }
    return distinct.size();
}

void ValidateCoordinates(const std::vector<Coordinate>& coordinates,
                         const ValidationOptions& options,
                         std::vector<Diagnostic>& diagnostics,
                         std::optional<bool>& dimensions,
                         std::optional<std::uint32_t> part = std::nullopt,
                         std::optional<std::uint32_t> ring = std::nullopt) {
    for (std::size_t index = 0; index < coordinates.size(); ++index) {
        const Coordinate& coordinate = coordinates[index];
        if (!IsFinite(coordinate)) {
            diagnostics.push_back(MakeDiagnostic(
                DiagnosticCode::InvalidCoordinate,
                "coordinate contains a non-finite component", part, ring,
                static_cast<std::uint64_t>(index)));
        }
        const bool coordinateHasZ = coordinate.z.has_value();
        if (!dimensions.has_value()) {
            dimensions = coordinateHasZ;
        } else if (options.strict && *dimensions != coordinateHasZ) {
            diagnostics.push_back(MakeDiagnostic(
                DiagnosticCode::InvalidCoordinate,
                "mixed coordinate dimensionality is not allowed in strict mode",
                part, ring, static_cast<std::uint64_t>(index)));
        }
    }
}

void ValidateLine(const LineString& line, const ValidationOptions& options,
                  std::vector<Diagnostic>& diagnostics,
                  std::optional<bool>& dimensions,
                  std::optional<std::uint32_t> part = std::nullopt) {
    ValidateCoordinates(line.coordinates, options, diagnostics, dimensions, part);
    if (DistinctCoordinateCount(line.coordinates) < 2) {
        diagnostics.push_back(MakeDiagnostic(
            DiagnosticCode::InsufficientCoordinates,
            "line has fewer than two distinct points", part));
    }
}

void ValidateRing(const Ring& ring, const ValidationOptions& options,
                  std::vector<Diagnostic>& diagnostics,
                  std::optional<bool>& dimensions,
                  std::optional<std::uint32_t> part,
                  std::uint32_t ringIndex) {
    ValidateCoordinates(ring, options, diagnostics, dimensions, part, ringIndex);
    if (DistinctCoordinateCount(ring) < 3) {
        diagnostics.push_back(MakeDiagnostic(
            DiagnosticCode::InsufficientCoordinates,
            "ring has fewer than three distinct points", part, ringIndex));
    }
}

void ValidatePolygon(const Polygon& polygon, const ValidationOptions& options,
                     std::vector<Diagnostic>& diagnostics,
                     std::optional<bool>& dimensions,
                     std::optional<std::uint32_t> part = std::nullopt) {
    if (polygon.outer.empty()) {
        diagnostics.push_back(MakeDiagnostic(
            DiagnosticCode::InsufficientCoordinates,
            "polygon is missing its outer ring", part, 0));
    } else {
        ValidateRing(polygon.outer, options, diagnostics, dimensions, part, 0);
    }
    for (std::size_t index = 0; index < polygon.holes.size(); ++index) {
        ValidateRing(polygon.holes[index], options, diagnostics, dimensions, part,
                     static_cast<std::uint32_t>(index + 1));
    }
}

}  // namespace

std::vector<Diagnostic> ValidateGeometry(const Geometry& geometry,
                                         const ValidationOptions& options) {
    std::vector<Diagnostic> diagnostics;
    std::optional<bool> dimensions;
    std::visit(
        [&diagnostics, &dimensions, &options](const auto& value) {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, Point>) {
                ValidateCoordinates({value.coordinate}, options, diagnostics,
                                     dimensions);
            } else if constexpr (std::is_same_v<Value, MultiPoint>) {
                if (value.coordinates.empty()) {
                    diagnostics.push_back(MakeDiagnostic(
                        DiagnosticCode::InsufficientCoordinates,
                        "multi geometry has no parts"));
                } else {
                    ValidateCoordinates(value.coordinates, options, diagnostics,
                                        dimensions);
                }
            } else if constexpr (std::is_same_v<Value, LineString>) {
                ValidateLine(value, options, diagnostics, dimensions);
            } else if constexpr (std::is_same_v<Value, MultiLineString>) {
                if (value.lines.empty()) {
                    diagnostics.push_back(MakeDiagnostic(
                        DiagnosticCode::InsufficientCoordinates,
                        "multi geometry has no parts"));
                } else {
                    for (std::size_t index = 0; index < value.lines.size(); ++index) {
                        ValidateLine(value.lines[index], options, diagnostics, dimensions,
                                     static_cast<std::uint32_t>(index));
                    }
                }
            } else if constexpr (std::is_same_v<Value, Polygon>) {
                ValidatePolygon(value, options, diagnostics, dimensions);
            } else if constexpr (std::is_same_v<Value, MultiPolygon>) {
                if (value.polygons.empty()) {
                    diagnostics.push_back(MakeDiagnostic(
                        DiagnosticCode::InsufficientCoordinates,
                        "multi geometry has no parts"));
                } else {
                    for (std::size_t index = 0; index < value.polygons.size(); ++index) {
                        ValidatePolygon(value.polygons[index], options, diagnostics, dimensions,
                                        static_cast<std::uint32_t>(index));
                    }
                }
            }
        },
        geometry);
    return diagnostics;
}

Result<Ring> NormalizeRing(const Ring& ring) {
    Ring normalized = ring;
    if (normalized.size() > 1 && normalized.front() == normalized.back()) {
        normalized.pop_back();
    }
    if (DistinctCoordinateCount(normalized) < 3) {
        return Result<Ring>::Failure({MakeDiagnostic(
            DiagnosticCode::InsufficientCoordinates,
            "ring has fewer than three distinct points")});
    }
    return Result<Ring>::Success(std::move(normalized));
}

}  // namespace usdvector
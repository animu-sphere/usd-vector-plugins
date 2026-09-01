#include "usdvector/authoring/authoring.h"

#include "usdvector/core/identifiers.h"
#include "usdvector/core/validation.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <set>
#include <type_traits>
#include <utility>

namespace usdvector::authoring {
namespace {

using Json = nlohmann::json;

Json ToJson(const PropertyValue& source) {
    return std::visit(
        [](const auto& value) -> Json {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::monostate>) {
                return nullptr;
            } else if constexpr (std::is_same_v<Value, PropertyValue::Array>) {
                Json result = Json::array();
                for (const PropertyValue& item : value) {
                    result.push_back(ToJson(item));
                }
                return result;
            } else if constexpr (std::is_same_v<Value, PropertyValue::Object>) {
                Json result = Json::object();
                for (const auto& [name, item] : value) {
                    result[name] = ToJson(item);
                }
                return result;
            } else {
                return Json(value);
            }
        },
        source.value);
}

template <typename Value, typename Array>
bool IsHomogeneousScalarArray(const Array& source) {
    for (const PropertyValue& item : source) {
        if (!std::holds_alternative<Value>(item.value)) {
            return false;
        }
    }
    return !source.empty();
}

template <typename Value>
std::vector<Value> CopyScalarArray(const PropertyValue::Array& source) {
    std::vector<Value> result;
    result.reserve(source.size());
    for (const PropertyValue& item : source) {
        result.push_back(std::get<Value>(item.value));
    }
    return result;
}

CanonicalJson ToCanonicalJson(const PropertyValue& source) {
    return {ToJson(source).dump()};
}

Diagnostic FeatureDiagnostic(Diagnostic diagnostic, std::size_t featureIndex,
                             const Feature& feature) {
    diagnostic.featureIndex = static_cast<std::uint64_t>(featureIndex);
    if (feature.id.has_value()) {
        diagnostic.featureId = MakeFeatureName(*feature.id);
    }
    return diagnostic;
}

std::string UniqueName(const std::string& base,
                       std::set<std::string>& usedNames,
                       std::vector<Diagnostic>& diagnostics,
                       std::size_t featureIndex, const Feature& feature,
                       bool strict) {
    if (usedNames.insert(base).second) {
        return base;
    }
    std::size_t suffix = 2;
    std::string uniqueName;
    do {
        uniqueName = base + "_" + std::to_string(suffix++);
    } while (!usedNames.insert(uniqueName).second);
    Diagnostic diagnostic = FeatureDiagnostic(
        {DiagnosticCode::DuplicateFeatureId,
         strict ? Severity::Error : Severity::Warning,
         "feature name collided; a deterministic suffix was added",
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt},
        featureIndex, feature);
    diagnostics.push_back(std::move(diagnostic));
    return uniqueName;
}

Diagnostic BoundsDiagnostic(std::size_t featureIndex, const Feature& feature,
                            bool strict) {
    return FeatureDiagnostic(
        {DiagnosticCode::BoundsMismatch,
         strict ? Severity::Error : Severity::Warning,
         "declared bounds disagree with computed bounds",
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt},
        featureIndex, feature);
}

bool BoundsEqual(const Bounds& left, const Bounds& right) {
    return left.empty == right.empty &&
           (left.empty || (left.minX == right.minX &&
                           left.minY == right.minY &&
                           left.maxX == right.maxX &&
                           left.maxY == right.maxY &&
                           left.minZ == right.minZ &&
                           left.maxZ == right.maxZ));
}

Diagnostic PrecisionDiagnostic(std::size_t featureIndex, const Feature& feature) {
    return FeatureDiagnostic(
        {DiagnosticCode::LocalCoordinatePrecision,
         Severity::Error,
         "local-coordinate conversion produced a non-finite component",
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt,
         std::nullopt},
        featureIndex, feature);
}

bool HasInvalidLocalCoordinates(const GeometryPlan& geometry) {
    for (const LocalCoordinate& point : geometry.points) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
            (point.z.has_value() && !std::isfinite(*point.z))) {
            return true;
        }
    }
    for (const GeometryPlan& child : geometry.children) {
        if (HasInvalidLocalCoordinates(child)) {
            return true;
        }
    }
    return false;
}

void NormalizeProperties(const Properties& source, FeaturePlan& target,
                         std::vector<Diagnostic>& diagnostics,
                         std::size_t featureIndex, const Feature& feature) {
    std::map<std::string, std::size_t> usedNames;
    for (const auto& [sourceName, value] : source) {
        const std::string base = NormalizeIdentifier(sourceName);
        const std::size_t occurrence = ++usedNames[base];
        const std::string normalized = occurrence == 1
                                           ? base
                                           : base + "_" + std::to_string(occurrence);
        target.properties.emplace(normalized, value);
        if (normalized != sourceName) {
            target.propertyNames.emplace(normalized, sourceName);
            diagnostics.push_back(FeatureDiagnostic(
                {DiagnosticCode::PropertyNameNormalized,
                 Severity::Warning,
                 "property name was normalized for USD",
                 std::nullopt,
                 std::nullopt,
                 std::nullopt,
                 std::nullopt,
                 std::nullopt,
                 std::nullopt,
                 normalized},
                featureIndex, feature));
        }
        if (std::holds_alternative<std::monostate>(value.value)) {
            target.nullProperties.push_back(normalized);
        }
    }
}

LocalCoordinate Local(const Coordinate& source, const LocalCoordinate& origin) {
    return {source.x - origin.x,
            source.y - origin.y,
            source.z.has_value()
                ? std::optional<double>{source.z.value() - origin.z.value_or(0.0)}
                : std::nullopt};
}

GeometryPlan MakeSimplePlan(GeometryType type,
                            const std::vector<Coordinate>& coordinates,
                            const LocalCoordinate& origin) {
    GeometryPlan plan;
    plan.sourceType = type;
    plan.points.reserve(coordinates.size());
    for (const Coordinate& coordinate : coordinates) {
        plan.points.push_back(Local(coordinate, origin));
    }
    return plan;
}

Result<GeometryPlan> MakeGeometryPlan(const Geometry& geometry,
                                      const LocalCoordinate& origin) {
    return std::visit(
        [&](const auto& value) -> Result<GeometryPlan> {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::monostate>) {
                return Result<GeometryPlan>::Success(
                    GeometryPlan{GeometryType::Null, {}, {}, {}, {}});
            } else if constexpr (std::is_same_v<Value, Point>) {
                return Result<GeometryPlan>::Success(MakeSimplePlan(
                    GeometryType::Point, {value.coordinate}, origin));
            } else if constexpr (std::is_same_v<Value, MultiPoint>) {
                return Result<GeometryPlan>::Success(MakeSimplePlan(
                    GeometryType::MultiPoint, value.coordinates, origin));
            } else if constexpr (std::is_same_v<Value, LineString>) {
                GeometryPlan plan = MakeSimplePlan(
                    GeometryType::LineString, value.coordinates, origin);
                plan.curveVertexCounts.push_back(
                    static_cast<std::uint32_t>(value.coordinates.size()));
                return Result<GeometryPlan>::Success(std::move(plan));
            } else if constexpr (std::is_same_v<Value, MultiLineString>) {
                GeometryPlan plan;
                plan.sourceType = GeometryType::MultiLineString;
                for (const LineString& line : value.lines) {
                    plan.curveVertexCounts.push_back(
                        static_cast<std::uint32_t>(line.coordinates.size()));
                    for (const Coordinate& coordinate : line.coordinates) {
                        plan.points.push_back(Local(coordinate, origin));
                    }
                }
                return Result<GeometryPlan>::Success(std::move(plan));
            } else if constexpr (std::is_same_v<Value, Polygon>) {
                auto mesh = TriangulatePolygon(value);
                if (!mesh.Succeeded()) {
                    return Result<GeometryPlan>::Failure(
                        std::move(mesh.diagnostics));
                }
                GeometryPlan plan;
                plan.sourceType = GeometryType::Polygon;
                for (const Coordinate& vertex : mesh.value->vertices) {
                    plan.points.push_back(Local(vertex, origin));
                }
                plan.triangles = std::move(mesh.value->triangles);
                return Result<GeometryPlan>::Success(std::move(plan));
            } else if constexpr (std::is_same_v<Value, MultiPolygon>) {
                GeometryPlan plan;
                plan.sourceType = GeometryType::MultiPolygon;
                for (std::size_t partIndex = 0; partIndex < value.polygons.size();
                     ++partIndex) {
                    const Polygon& polygon = value.polygons[partIndex];
                    auto child = MakeGeometryPlan(Geometry{polygon}, origin);
                    if (!child.Succeeded()) {
                        for (Diagnostic& diagnostic : child.diagnostics) {
                            if (!diagnostic.partIndex.has_value()) {
                                diagnostic.partIndex =
                                    static_cast<std::uint32_t>(partIndex);
                            }
                        }
                        return Result<GeometryPlan>::Failure(
                            std::move(child.diagnostics));
                    }
                    plan.children.push_back(std::move(*child.value));
                }
                return Result<GeometryPlan>::Success(std::move(plan));
            }
        },
        geometry);
}

}  // namespace

LocalCoordinate ToLocalCoordinate(const Coordinate& source,
                                  const LocalCoordinate& origin) {
    return Local(source, origin);
}

std::optional<UsdPropertyValue> ToUsdPropertyValue(
    const PropertyValue& source) {
    return std::visit(
        [&](const auto& value) -> std::optional<UsdPropertyValue> {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::monostate>) {
                return std::nullopt;
            } else if constexpr (std::is_same_v<Value, PropertyValue::Array>) {
                if (IsHomogeneousScalarArray<bool>(value)) {
                    return CopyScalarArray<bool>(value);
                }
                if (IsHomogeneousScalarArray<std::int64_t>(value)) {
                    return CopyScalarArray<std::int64_t>(value);
                }
                if (IsHomogeneousScalarArray<std::uint64_t>(value)) {
                    return CopyScalarArray<std::uint64_t>(value);
                }
                if (IsHomogeneousScalarArray<double>(value)) {
                    return CopyScalarArray<double>(value);
                }
                if (IsHomogeneousScalarArray<std::string>(value)) {
                    return CopyScalarArray<std::string>(value);
                }
                return ToCanonicalJson(source);
            } else if constexpr (std::is_same_v<Value, PropertyValue::Object>) {
                return ToCanonicalJson(source);
            } else {
                return value;
            }
        },
        source.value);
}

UsdProperties ToUsdProperties(const Properties& source) {
    UsdProperties result;
    for (const auto& [name, value] : source) {
        const std::optional<UsdPropertyValue> converted =
            ToUsdPropertyValue(value);
        if (converted.has_value()) {
            result.emplace(name, *converted);
        }
    }
    return result;
}

Result<AuthoringPlan> BuildAuthoringPlan(
    const DatasetMetadata& metadata, const std::vector<Feature>& features,
    const AuthoringOptions& options) {
    AuthoringPlan plan;
    plan.metadata = metadata;
    plan.sourceBounds = ComputeBounds(features);
    const Coordinate sourceOrigin = plan.sourceBounds.Center();
    plan.localOrigin = {sourceOrigin.x, sourceOrigin.y, sourceOrigin.z};

    std::vector<Diagnostic> diagnostics;
    if (metadata.declaredBounds.has_value() &&
        !BoundsEqual(*metadata.declaredBounds, plan.sourceBounds)) {
        diagnostics.push_back({DiagnosticCode::BoundsMismatch,
                               options.strict ? Severity::Error : Severity::Warning,
                               "declared bounds disagree with computed bounds",
                               std::nullopt,
                               std::nullopt,
                               std::nullopt,
                               std::nullopt,
                               std::nullopt,
                               std::nullopt,
                               std::nullopt});
    }

    std::set<std::string> usedFeatureNames;
    for (std::size_t featureIndex = 0; featureIndex < features.size(); ++featureIndex) {
        const Feature& feature = features[featureIndex];
        const auto validation = ValidateGeometry(
            feature.geometry, ValidationOptions{options.strict});
        if (!validation.empty()) {
            for (Diagnostic diagnostic : validation) {
                diagnostics.push_back(
                    FeatureDiagnostic(std::move(diagnostic), featureIndex, feature));
            }
            continue;
        }

        if (feature.declaredBounds.has_value() &&
            !BoundsEqual(*feature.declaredBounds,
                         ComputeBounds(feature.geometry))) {
            diagnostics.push_back(
                BoundsDiagnostic(featureIndex, feature, options.strict));
        }

        auto geometry = MakeGeometryPlan(feature.geometry, plan.localOrigin);
        if (!geometry.Succeeded()) {
            for (Diagnostic diagnostic : geometry.diagnostics) {
                diagnostics.push_back(
                    FeatureDiagnostic(std::move(diagnostic), featureIndex, feature));
            }
            continue;
        }
        if (HasInvalidLocalCoordinates(*geometry.value)) {
            diagnostics.push_back(PrecisionDiagnostic(featureIndex, feature));
            continue;
        }

        FeaturePlan featurePlan;
        featurePlan.sourceId = feature.id;
        featurePlan.name = UniqueName(
            feature.id.has_value() ? MakeFeatureName(*feature.id)
                                   : MakeFeatureName(featureIndex),
            usedFeatureNames, diagnostics, featureIndex, feature, options.strict);
        featurePlan.geometry = std::move(*geometry.value);
        featurePlan.foreignMembers = feature.foreignMembers;
        NormalizeProperties(feature.properties, featurePlan, diagnostics,
                            featureIndex, feature);
        plan.features.push_back(std::move(featurePlan));
    }

    const bool hasErrors = std::any_of(
        diagnostics.begin(), diagnostics.end(), [](const Diagnostic& diagnostic) {
            return diagnostic.severity == Severity::Error;
        });
    if (hasErrors) {
        return Result<AuthoringPlan>::Failure(std::move(diagnostics));
    }
    Result<AuthoringPlan> result = Result<AuthoringPlan>::Success(std::move(plan));
    result.diagnostics = std::move(diagnostics);
    return result;
}

}  // namespace usdvector::authoring

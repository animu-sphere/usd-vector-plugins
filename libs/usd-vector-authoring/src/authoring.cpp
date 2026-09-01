#include "usdvector/authoring/authoring.h"

#include "usdvector/core/identifiers.h"
#include "usdvector/core/validation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace usdvector::authoring {
namespace {

Diagnostic FeatureDiagnostic(Diagnostic diagnostic, std::size_t featureIndex,
                             const Feature& feature) {
    diagnostic.featureIndex = static_cast<std::uint64_t>(featureIndex);
    if (feature.id.has_value()) {
        diagnostic.featureId = MakeFeatureName(*feature.id);
    }
    return diagnostic;
}

std::string UniqueName(const std::string& base,
                       std::map<std::string, std::size_t>& usedNames,
                       std::vector<Diagnostic>& diagnostics,
                       std::size_t featureIndex, const Feature& feature) {
    const std::size_t occurrence = ++usedNames[base];
    if (occurrence == 1) {
        return base;
    }
    Diagnostic diagnostic = FeatureDiagnostic(
        {DiagnosticCode::DuplicateFeatureId,
         Severity::Warning,
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
    return base + "_" + std::to_string(occurrence);
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
                for (const Polygon& polygon : value.polygons) {
                    auto child = MakeGeometryPlan(Geometry{polygon}, origin);
                    if (!child.Succeeded()) {
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

Result<AuthoringPlan> BuildAuthoringPlan(
    const DatasetMetadata& metadata, const std::vector<Feature>& features,
    const AuthoringOptions& options) {
    AuthoringPlan plan;
    plan.metadata = metadata;
    plan.sourceBounds = ComputeBounds(features);
    const Coordinate sourceOrigin = plan.sourceBounds.Center();
    plan.localOrigin = {sourceOrigin.x, sourceOrigin.y, sourceOrigin.z};

    std::map<std::string, std::size_t> usedFeatureNames;
    std::vector<Diagnostic> diagnostics;
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

        auto geometry = MakeGeometryPlan(feature.geometry, plan.localOrigin);
        if (!geometry.Succeeded()) {
            for (Diagnostic diagnostic : geometry.diagnostics) {
                diagnostics.push_back(
                    FeatureDiagnostic(std::move(diagnostic), featureIndex, feature));
            }
            continue;
        }

        FeaturePlan featurePlan;
        featurePlan.sourceId = feature.id;
        featurePlan.name = UniqueName(
            feature.id.has_value() ? MakeFeatureName(*feature.id)
                                   : MakeFeatureName(featureIndex),
            usedFeatureNames, diagnostics, featureIndex, feature);
        featurePlan.geometry = std::move(*geometry.value);
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

#pragma once

#include "usdvector/core/bounds.h"
#include "usdvector/core/diagnostics.h"
#include "usdvector/core/model.h"
#include "usdvector/authoring/triangulation.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace usdvector::authoring {

struct LocalCoordinate {
    double x = 0.0;
    double y = 0.0;
    std::optional<double> z;
};

struct GeometryPlan {
    GeometryType sourceType = GeometryType::Null;
    std::vector<LocalCoordinate> points;
    std::vector<std::uint32_t> curveVertexCounts;
    std::vector<Triangle> triangles;
    std::vector<GeometryPlan> children;
};

struct FeaturePlan {
    std::string name;
    std::optional<FeatureId> sourceId;
    GeometryPlan geometry;
    Properties properties;
    std::map<std::string, std::string> propertyNames;
    std::vector<std::string> nullProperties;
};

struct CanonicalJson {
    std::string text;
};

using UsdPropertyValue = std::variant<
    bool, std::int64_t, std::uint64_t, double, std::string,
    std::vector<bool>, std::vector<std::int64_t>, std::vector<std::uint64_t>,
    std::vector<double>, std::vector<std::string>, CanonicalJson>;
using UsdProperties = std::map<std::string, UsdPropertyValue>;

struct AuthoringPlan {
    DatasetMetadata metadata;
    Bounds sourceBounds;
    LocalCoordinate localOrigin;
    std::vector<FeaturePlan> features;
};

struct AuthoringOptions {
    bool strict = false;
};

Result<AuthoringPlan> BuildAuthoringPlan(
    const DatasetMetadata& metadata, const std::vector<Feature>& features,
    const AuthoringOptions& options = {});

LocalCoordinate ToLocalCoordinate(const Coordinate& source,
                                  const LocalCoordinate& origin);

std::optional<UsdPropertyValue> ToUsdPropertyValue(
    const PropertyValue& source);
UsdProperties ToUsdProperties(const Properties& source);

}  // namespace usdvector::authoring

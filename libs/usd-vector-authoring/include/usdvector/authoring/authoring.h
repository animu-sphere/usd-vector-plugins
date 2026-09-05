#pragma once

#include "usdvector/core/bounds.h"
#include "usdvector/core/diagnostics.h"
#include "usdvector/core/model.h"
#include "usdvector/authoring/triangulation.h"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
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
    ForeignMembers foreignMembers;
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

using FeaturePlanSink = std::function<void(FeaturePlan&&)>;

class FeaturePlanBuilder {
public:
    FeaturePlanBuilder(const DatasetMetadata& metadata,
                       const Bounds& sourceBounds, FeaturePlanSink sink,
                       const AuthoringOptions& options = {});

    void Add(const Feature& feature);
    Result<std::monostate> Finish();

private:
    Bounds sourceBounds_;
    LocalCoordinate localOrigin_;
    FeaturePlanSink sink_;
    AuthoringOptions options_;
    std::set<std::string> usedFeatureNames_;
    std::vector<Diagnostic> diagnostics_;
    std::size_t nextFeatureIndex_ = 0;
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

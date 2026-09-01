#pragma once

#include "usdvector/core/bounds.h"
#include "usdvector/core/geometry.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <utility>

namespace usdvector {

struct PropertyValue {
    using Array = std::vector<PropertyValue>;
    using Object = std::map<std::string, PropertyValue>;
    using Value = std::variant<std::monostate, bool, std::int64_t, std::uint64_t,
                               double, std::string, Array, Object>;

    Value value;

    PropertyValue() = default;
    explicit PropertyValue(Value source) : value(std::move(source)) {}
};

using Properties = std::map<std::string, PropertyValue>;
using FeatureId = std::variant<std::string, std::int64_t, std::uint64_t>;

struct Feature {
    std::optional<FeatureId> id;
    Geometry geometry;
    Properties properties;
    std::optional<Bounds> declaredBounds;
};

struct DatasetMetadata {
    std::string format;
    std::optional<Bounds> computedBounds;
    std::optional<Bounds> declaredBounds;
    std::optional<std::string> crs;
    std::optional<std::uint64_t> featureCount;
};

}  // namespace usdvector
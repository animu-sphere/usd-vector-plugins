#include "usdvector/authoring/usd_authoring.h"

#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/value.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xform.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

PXR_NAMESPACE_USING_DIRECTIVE

namespace usdvector::authoring {
namespace {

Diagnostic UsdError(const std::string& message,
                    std::optional<std::size_t> featureIndex = std::nullopt) {
    Diagnostic diagnostic{DiagnosticCode::UsdAuthoringFailed,
                          Severity::Error,
                          message,
                          std::nullopt,
                          std::nullopt,
                          std::nullopt,
                          std::nullopt,
                          std::nullopt,
                          std::nullopt,
                          std::nullopt};
    if (featureIndex.has_value()) {
        diagnostic.featureIndex = static_cast<std::uint64_t>(*featureIndex);
    }
    return diagnostic;
}

GfVec3f ToFloatPoint(const LocalCoordinate& point, bool& valid) {
    const float x = static_cast<float>(point.x);
    const float y = static_cast<float>(point.y);
    const float z = static_cast<float>(point.z.value_or(0.0));
    valid = std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    return {x, y, z};
}

VtArray<GfVec3f> ToPoints(const GeometryPlan& geometry, bool& valid) {
    VtArray<GfVec3f> points;
    points.reserve(geometry.points.size());
    valid = true;
    for (const LocalCoordinate& point : geometry.points) {
        bool pointValid = false;
        points.push_back(ToFloatPoint(point, pointValid));
        valid = valid && pointValid;
    }
    return points;
}

bool SetAttribute(const UsdAttribute& attribute, const VtValue& value) {
    return attribute.IsValid() && attribute.Set(value);
}

bool AuthorPoints(const UsdStageRefPtr& stage, const SdfPath& path,
                  const GeometryPlan& geometry) {
    bool valid = false;
    VtArray<GfVec3f> points = ToPoints(geometry, valid);
    if (!valid) {
        return false;
    }
    const UsdGeomPoints prim = UsdGeomPoints::Define(stage, path);
    return SetAttribute(prim.CreatePointsAttr(), VtValue(points));
}

bool AuthorCurves(const UsdStageRefPtr& stage, const SdfPath& path,
                  const GeometryPlan& geometry) {
    bool valid = false;
    VtArray<GfVec3f> points = ToPoints(geometry, valid);
    if (!valid) {
        return false;
    }
    VtArray<std::int32_t> counts;
    counts.reserve(geometry.curveVertexCounts.size());
    for (std::uint32_t count : geometry.curveVertexCounts) {
        if (count > static_cast<std::uint32_t>(
                        std::numeric_limits<std::int32_t>::max())) {
            return false;
        }
        counts.push_back(static_cast<std::int32_t>(count));
    }

    const UsdGeomBasisCurves prim = UsdGeomBasisCurves::Define(stage, path);
    return SetAttribute(prim.CreatePointsAttr(), VtValue(points)) &&
           SetAttribute(prim.CreateCurveVertexCountsAttr(), VtValue(counts)) &&
           SetAttribute(prim.CreateTypeAttr(), VtValue(UsdGeomTokens->linear)) &&
           SetAttribute(prim.CreateWrapAttr(), VtValue(UsdGeomTokens->nonperiodic));
}

bool AuthorMesh(const UsdStageRefPtr& stage, const SdfPath& path,
                const GeometryPlan& geometry) {
    bool valid = false;
    VtArray<GfVec3f> points = ToPoints(geometry, valid);
    if (!valid) {
        return false;
    }
    VtArray<std::int32_t> counts;
    VtArray<std::int32_t> indices;
    if (geometry.points.size() >
        static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
        return false;
    }
    counts.reserve(geometry.triangles.size());
    indices.reserve(geometry.triangles.size() * 3);
    for (const Triangle& triangle : geometry.triangles) {
        if (triangle.a >= geometry.points.size() ||
            triangle.b >= geometry.points.size() ||
            triangle.c >= geometry.points.size()) {
            return false;
        }
        counts.push_back(3);
        indices.push_back(static_cast<std::int32_t>(triangle.a));
        indices.push_back(static_cast<std::int32_t>(triangle.b));
        indices.push_back(static_cast<std::int32_t>(triangle.c));
    }

    const UsdGeomMesh prim = UsdGeomMesh::Define(stage, path);
    return SetAttribute(prim.CreatePointsAttr(), VtValue(points)) &&
           SetAttribute(prim.CreateFaceVertexCountsAttr(), VtValue(counts)) &&
           SetAttribute(prim.CreateFaceVertexIndicesAttr(), VtValue(indices)) &&
           SetAttribute(prim.CreateSubdivisionSchemeAttr(),
                        VtValue(UsdGeomTokens->none));
}

bool AuthorGeometry(const UsdStageRefPtr& stage, const SdfPath& path,
                    const GeometryPlan& geometry) {
    switch (geometry.sourceType) {
        case GeometryType::Null:
            return UsdGeomXform::Define(stage, path).GetPrim().IsValid();
        case GeometryType::Point:
        case GeometryType::MultiPoint:
            return AuthorPoints(stage, path, geometry);
        case GeometryType::LineString:
        case GeometryType::MultiLineString:
            return AuthorCurves(stage, path, geometry);
        case GeometryType::Polygon:
            return AuthorMesh(stage, path, geometry);
        case GeometryType::MultiPolygon: {
            if (!UsdGeomXform::Define(stage, path).GetPrim().IsValid()) {
                return false;
            }
            for (std::size_t index = 0; index < geometry.children.size(); ++index) {
                const SdfPath childPath = path.AppendChild(
                    TfToken("part_" + std::to_string(index)));
                if (!AuthorGeometry(stage, childPath, geometry.children[index])) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

template <typename Value>
VtArray<Value> ToArray(const std::vector<Value>& source) {
    VtArray<Value> result;
    result.reserve(source.size());
    for (const Value& item : source) {
        result.push_back(item);
    }
    return result;
}

bool AuthorProperty(const UsdPrim& prim, const std::string& name,
                    const UsdPropertyValue& value) {
    const std::string attributeName = "vector:properties:" + name;
    return std::visit(
        [&](const auto& item) {
            using Value = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<Value, bool>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->Bool, true),
                                    VtValue(item));
            } else if constexpr (std::is_same_v<Value, std::int64_t>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->Int64, true),
                                    VtValue(item));
            } else if constexpr (std::is_same_v<Value, std::uint64_t>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->UInt64, true),
                                    VtValue(item));
            } else if constexpr (std::is_same_v<Value, double>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->Double, true),
                                    VtValue(item));
            } else if constexpr (std::is_same_v<Value, std::string>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->String, true),
                                    VtValue(item));
            } else if constexpr (std::is_same_v<Value, std::vector<bool>>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->BoolArray, true),
                                    VtValue(ToArray(item)));
            } else if constexpr (std::is_same_v<Value, std::vector<std::int64_t>>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->Int64Array, true),
                                    VtValue(ToArray(item)));
            } else if constexpr (std::is_same_v<Value, std::vector<std::uint64_t>>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->UInt64Array, true),
                                    VtValue(ToArray(item)));
            } else if constexpr (std::is_same_v<Value, std::vector<double>>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->DoubleArray, true),
                                    VtValue(ToArray(item)));
            } else if constexpr (std::is_same_v<Value, std::vector<std::string>>) {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->StringArray, true),
                                    VtValue(ToArray(item)));
            } else {
                return SetAttribute(prim.CreateAttribute(
                                        TfToken(attributeName),
                                        SdfValueTypeNames->String, true),
                                    VtValue(item.text));
            }
        },
        value);
}

bool AuthorFeatureMetadata(const UsdPrim& prim, const FeaturePlan& feature) {
    if (feature.sourceId.has_value()) {
        const VtValue sourceId = std::visit(
            [](const auto& value) { return VtValue(value); },
            *feature.sourceId);
        prim.SetCustomDataByKey(TfToken("vector:featureId"), sourceId);
    }
    if (!feature.nullProperties.empty()) {
        VtArray<TfToken> nullProperties;
        for (const std::string& name : feature.nullProperties) {
            nullProperties.push_back(TfToken(name));
        }
        prim.SetCustomDataByKey(TfToken("vector:nullProperties"),
                                VtValue(nullProperties));
    }
    if (!feature.propertyNames.empty()) {
        VtDictionary propertyNames;
        for (const auto& [normalized, original] : feature.propertyNames) {
            propertyNames[TfToken(normalized)] = VtValue(original);
        }
        prim.SetCustomDataByKey(TfToken("vector:propertyNames"),
                                VtValue(propertyNames));
    }
    if (!feature.foreignMembers.empty()) {
        const auto foreignMembers = ToUsdPropertyValue(
            PropertyValue{PropertyValue::Object(feature.foreignMembers)});
        if (foreignMembers.has_value() &&
            std::holds_alternative<CanonicalJson>(*foreignMembers)) {
            prim.SetCustomDataByKey(
                TfToken("vector:foreignMembers"),
                VtValue(std::get<CanonicalJson>(*foreignMembers).text));
        }
    }
    const UsdProperties properties = ToUsdProperties(feature.properties);
    for (const auto& [name, value] : properties) {
        if (!AuthorProperty(prim, name, value)) {
            return false;
        }
    }
    return true;
}

bool AuthorDatasetMetadata(const UsdPrim& prim, const AuthoringPlan& plan) {
    prim.SetCustomDataByKey(TfToken("vector:format"),
                            VtValue(plan.metadata.format));
    if (!plan.sourceBounds.empty) {
        VtArray<double> bounds;
        bounds.push_back(plan.sourceBounds.minX);
        bounds.push_back(plan.sourceBounds.minY);
        bounds.push_back(plan.sourceBounds.maxX);
        bounds.push_back(plan.sourceBounds.maxY);
        if (plan.sourceBounds.minZ.has_value()) {
            bounds.push_back(*plan.sourceBounds.minZ);
            bounds.push_back(*plan.sourceBounds.maxZ);
        }
        prim.SetCustomDataByKey(TfToken("vector:sourceBounds"),
                                VtValue(bounds));
    }
    const VtValue origin = VtValue(GfVec3d(
        plan.localOrigin.x, plan.localOrigin.y, plan.localOrigin.z.value_or(0.0)));
    prim.SetCustomDataByKey(TfToken("vector:localOrigin"), origin);
    if (plan.metadata.crs.has_value()) {
        prim.SetCustomDataByKey(TfToken("vector:crs"),
                                VtValue(*plan.metadata.crs));
    }
    if (!plan.metadata.foreignMembers.empty()) {
        const auto foreignMembers = ToUsdPropertyValue(
            PropertyValue{PropertyValue::Object(plan.metadata.foreignMembers)});
        if (foreignMembers.has_value() &&
            std::holds_alternative<CanonicalJson>(*foreignMembers)) {
            prim.SetCustomDataByKey(
                TfToken("vector:foreignMembers"),
                VtValue(std::get<CanonicalJson>(*foreignMembers).text));
        }
    }
    if (plan.metadata.featureCount.has_value()) {
        prim.SetCustomDataByKey(TfToken("vector:featureCount"),
                                VtValue(*plan.metadata.featureCount));
    }
    return true;
}

}  // namespace

Result<UsdStageRefPtr> BuildUsdStage(const AuthoringPlan& plan,
                                    const StageOptions& options) {
    if (options.metersPerUnit.has_value() &&
        !(std::isfinite(*options.metersPerUnit) &&
          *options.metersPerUnit > 0.0)) {
        return Result<UsdStageRefPtr>::Failure(
            {UsdError("metersPerUnit must be a positive finite number")});
    }

    const UsdStageRefPtr stage = UsdStage::CreateInMemory();
    if (!stage) {
        return Result<UsdStageRefPtr>::Failure(
            {UsdError("could not create an in-memory USD stage")});
    }

    const UsdGeomXform vector = UsdGeomXform::Define(stage, SdfPath("/Vector"));
    const UsdGeomXform features =
        UsdGeomXform::Define(stage, SdfPath("/Vector/Features"));
    if (!vector.GetPrim().IsValid() || !features.GetPrim().IsValid() ||
        !AuthorDatasetMetadata(vector.GetPrim(), plan)) {
        return Result<UsdStageRefPtr>::Failure(
            {UsdError("could not author dataset metadata")});
    }
    stage->SetDefaultPrim(vector.GetPrim());
    if (options.upAxis.has_value()) {
        UsdGeomSetStageUpAxis(stage, *options.upAxis == StageUpAxis::Z
                                         ? UsdGeomTokens->z
                                         : UsdGeomTokens->y);
    }
    if (options.metersPerUnit.has_value()) {
        UsdGeomSetStageMetersPerUnit(stage, *options.metersPerUnit);
    }

    const UsdGeomXformable vectorXform(vector.GetPrim());
    const UsdGeomXformOp translate =
        vectorXform.AddTranslateOp(UsdGeomXformOp::PrecisionDouble);
    if (!translate.Set(GfVec3d(plan.localOrigin.x, plan.localOrigin.y,
                               plan.localOrigin.z.value_or(0.0)))) {
        return Result<UsdStageRefPtr>::Failure(
            {UsdError("could not author dataset local origin")});
    }

    for (std::size_t index = 0; index < plan.features.size(); ++index) {
        const FeaturePlan& feature = plan.features[index];
        const SdfPath featurePath =
            SdfPath("/Vector/Features").AppendChild(TfToken(feature.name));
        if (!AuthorGeometry(stage, featurePath, feature.geometry)) {
            return Result<UsdStageRefPtr>::Failure(
                {UsdError("could not author feature geometry", index)});
        }
        const UsdPrim featurePrim = stage->GetPrimAtPath(featurePath);
        if (!featurePrim.IsValid() || !AuthorFeatureMetadata(featurePrim, feature)) {
            return Result<UsdStageRefPtr>::Failure(
                {UsdError("could not author feature metadata", index)});
        }
    }
    return Result<UsdStageRefPtr>::Success(stage);
}

}  // namespace usdvector::authoring
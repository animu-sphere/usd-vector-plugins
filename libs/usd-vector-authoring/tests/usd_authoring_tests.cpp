#include "usdvector/authoring/usd_authoring.h"

#include <pxr/base/gf/vec3d.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <cassert>
#include <cstdint>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

void TestStageMapping() {
    usdvector::Feature feature;
    feature.id = usdvector::FeatureId{std::string{"road 1"}};
    feature.geometry = usdvector::Geometry{usdvector::Polygon{
        {{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}}, {}}};
    feature.properties.emplace("road name",
                               usdvector::PropertyValue{std::int64_t{7}});
    const auto plan = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 1},
        {feature});
    assert(plan.Succeeded());

    const auto stage = usdvector::authoring::BuildUsdStage(*plan.value);
    assert(stage.Succeeded());
    const pxr::UsdPrim vector = stage.value->GetPrimAtPath(pxr::SdfPath("/Vector"));
    const pxr::UsdPrim mesh = stage.value->GetPrimAtPath(
        pxr::SdfPath("/Vector/Features/id_road_1"));
    assert(vector.IsValid());
    assert(mesh.IsValid());
    assert(mesh.IsA<pxr::UsdGeomMesh>());
    std::int64_t roadName = 0;
    assert(mesh.GetAttribute(pxr::TfToken("vector:properties:road_name"))
               .Get(&roadName));
    assert(roadName == 7);
    const pxr::VtValue origin =
        vector.GetCustomDataByKey(pxr::TfToken("vector:localOrigin"));
    assert(!origin.IsEmpty());
    assert(origin.Get<pxr::GfVec3d>() == pxr::GfVec3d(1.0, 1.0, 0.0));
}

void TestLinearCurveMapping() {
    usdvector::Feature feature;
    feature.id = usdvector::FeatureId{std::string{"line 1"}};
    feature.geometry = usdvector::Geometry{usdvector::LineString{
        {{0.0, 0.0}, {1.0, 1.0}, {2.0, 0.0}}}};
    const auto plan = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 1},
        {feature});
    assert(plan.Succeeded());

    const auto stage = usdvector::authoring::BuildUsdStage(*plan.value);
    assert(stage.Succeeded());
    const pxr::UsdPrim curve = stage.value->GetPrimAtPath(
        pxr::SdfPath("/Vector/Features/id_line_1"));
    assert(curve.IsA<pxr::UsdGeomBasisCurves>());

    pxr::TfToken type;
    pxr::TfToken basis;
    assert(curve.GetAttribute(pxr::TfToken("type")).Get(&type));
    assert(type == pxr::UsdGeomTokens->linear);
    assert(curve.GetAttribute(pxr::TfToken("basis")).Get(&basis));
    assert(basis == pxr::UsdGeomTokens->bezier);
    assert(!curve.GetAttribute(pxr::TfToken("basis")).HasAuthoredValue());
}

}  // namespace

int main() {
    TestStageMapping();
    TestLinearCurveMapping();
}
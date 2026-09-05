#include "usdvector/authoring/authoring.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <variant>

namespace {

void TestLocalOriginPreservesSmallDifferences() {
    usdvector::Feature feature;
    feature.geometry = usdvector::Geometry{usdvector::LineString{{
        {1000000000.125, -2000000000.5, 12.0},
        {1000000000.375, -2000000000.0, 18.0}}}};
    auto result = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 1},
        {feature});
    assert(result.Succeeded());
    assert(result.value->localOrigin.x == 1000000000.25);
    assert(result.value->localOrigin.y == -2000000000.25);
    const auto& points = result.value->features[0].geometry.points;
    assert(points[0].x == -0.125);
    assert(points[1].x == 0.125);
    assert(points[0].z == -3.0);
    assert(points[1].z == 3.0);
}

void TestPolygonHoleTriangulation() {
    usdvector::Polygon polygon{
        {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}, {0.0, 0.0}},
        {{{2.0, 2.0}, {2.0, 8.0}, {8.0, 8.0}, {8.0, 2.0}, {2.0, 2.0}}}};
    auto result = usdvector::authoring::TriangulatePolygon(polygon);
    assert(result.Succeeded());
    assert(result.value->triangles.size() == 8);

    double area = 0.0;
    for (const auto& triangle : result.value->triangles) {
        const auto& a = result.value->vertices[triangle.a];
        const auto& b = result.value->vertices[triangle.b];
        const auto& c = result.value->vertices[triangle.c];
        area += std::abs((b.x - a.x) * (c.y - a.y) -
                         (b.y - a.y) * (c.x - a.x)) /
                2.0;
    }
    assert(std::abs(area - 64.0) < 1e-9);

    polygon.holes.push_back(
        {{1.0, 1.0}, {1.0, 1.5}, {1.5, 1.5}, {1.5, 1.0}, {1.0, 1.0}});
    auto multipleHoles = usdvector::authoring::TriangulatePolygon(polygon);
    assert(multipleHoles.Succeeded());
    assert(multipleHoles.value->triangles.size() == 14);
    double multipleHoleArea = 0.0;
    for (const auto& triangle : multipleHoles.value->triangles) {
        const auto& a = multipleHoles.value->vertices[triangle.a];
        const auto& b = multipleHoles.value->vertices[triangle.b];
        const auto& c = multipleHoles.value->vertices[triangle.c];
        multipleHoleArea +=
            std::abs((b.x - a.x) * (c.y - a.y) -
                     (b.y - a.y) * (c.x - a.x)) /
            2.0;
    }
    assert(std::abs(multipleHoleArea - 63.75) < 1e-9);
}

void TestNamesAndPropertiesAreDeterministic() {
    usdvector::Feature first;
    first.id = usdvector::FeatureId{"road name"};
    first.geometry = usdvector::Geometry{usdvector::Point{{1.0, 2.0}}};
    first.properties.emplace("road name", usdvector::PropertyValue{true});

    usdvector::Feature second = first;
    auto result = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 2},
        {first, second});
    assert(result.Succeeded());
    assert(result.value->features[0].name == "id_road_name");
    assert(result.value->features[1].name == "id_road_name_2");
    assert(result.value->features[0].propertyNames.at("road_name") ==
           "road name");
    assert(std::holds_alternative<bool>(
        result.value->features[0].properties.at("road_name").value));
    assert(result.diagnostics.size() == 3);
}

void TestFeaturePlanBuilderPreservesBatchOrderAndMapping() {
    usdvector::Feature first;
    first.id = usdvector::FeatureId{"same"};
    first.geometry = usdvector::Geometry{usdvector::Point{{0.0, 0.0}}};
    usdvector::Feature second;
    second.id = usdvector::FeatureId{"same"};
    second.geometry = usdvector::Geometry{usdvector::Point{{2.0, 2.0}}};

    usdvector::Bounds sourceBounds;
    sourceBounds.Include({0.0, 0.0});
    sourceBounds.Include({2.0, 2.0});
    std::vector<usdvector::authoring::FeaturePlan> streamed;
    usdvector::authoring::FeaturePlanBuilder builder(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 2},
        sourceBounds,
        [&streamed](usdvector::authoring::FeaturePlan&& feature) {
            streamed.push_back(std::move(feature));
        });
    builder.Add(first);
    builder.Add(second);
    const auto streamedResult = builder.Finish();

    assert(streamedResult.Succeeded());
    assert(streamed.size() == 2);
    assert(streamed[0].name == "id_same");
    assert(streamed[1].name == "id_same_2");
    assert(streamed[0].geometry.points[0].x == -1.0);
    assert(streamed[1].geometry.points[0].x == 1.0);
    assert(streamedResult.diagnostics.size() == 1);
    assert(streamedResult.diagnostics[0].code ==
           usdvector::DiagnosticCode::DuplicateFeatureId);

    const auto buffered = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 2},
        {first, second});
    assert(buffered.Succeeded());
    assert(buffered.value->features.size() == streamed.size());
    assert(buffered.value->features[0].name == streamed[0].name);
    assert(buffered.value->features[1].name == streamed[1].name);
    assert(buffered.value->features[0].geometry.points[0].x ==
           streamed[0].geometry.points[0].x);
    assert(buffered.value->features[1].geometry.points[0].x ==
           streamed[1].geometry.points[0].x);
}

void TestUsdPropertyMappingUsesTypedValuesAndCanonicalFallback() {
    usdvector::Properties properties;
    properties.emplace("flag", usdvector::PropertyValue{true});
    properties.emplace("signed", usdvector::PropertyValue{std::int64_t{-3}});
    properties.emplace("unsigned", usdvector::PropertyValue{std::uint64_t{4}});
    properties.emplace("number", usdvector::PropertyValue{2.5});
    properties.emplace("name", usdvector::PropertyValue{std::string{"road"}});
    properties.emplace(
        "numbers",
        usdvector::PropertyValue{usdvector::PropertyValue::Array{
            usdvector::PropertyValue{std::int64_t{1}},
            usdvector::PropertyValue{std::int64_t{2}}}});
    properties.emplace(
        "flags",
        usdvector::PropertyValue{usdvector::PropertyValue::Array{
            usdvector::PropertyValue{true},
            usdvector::PropertyValue{false}}});
    properties.emplace(
        "unsigneds",
        usdvector::PropertyValue{usdvector::PropertyValue::Array{
            usdvector::PropertyValue{std::uint64_t{3}},
            usdvector::PropertyValue{std::uint64_t{4}}}});
    properties.emplace(
        "doubles",
        usdvector::PropertyValue{usdvector::PropertyValue::Array{
            usdvector::PropertyValue{1.5}, usdvector::PropertyValue{2.5}}});
    properties.emplace(
        "names",
        usdvector::PropertyValue{usdvector::PropertyValue::Array{
            usdvector::PropertyValue{std::string{"first"}},
            usdvector::PropertyValue{std::string{"second"}}}});
    properties.emplace(
        "empty",
        usdvector::PropertyValue{usdvector::PropertyValue::Array{}});
    properties.emplace(
        "mixed",
        usdvector::PropertyValue{usdvector::PropertyValue::Array{
            usdvector::PropertyValue{std::int64_t{1}},
            usdvector::PropertyValue{2.0}}});
    properties.emplace(
        "object",
        usdvector::PropertyValue{usdvector::PropertyValue::Object{
            {"z", usdvector::PropertyValue{std::int64_t{1}}},
            {"a", usdvector::PropertyValue{true}}}});
    properties.emplace("missing", usdvector::PropertyValue{});

    const auto mapped = usdvector::authoring::ToUsdProperties(properties);
    assert(mapped.size() == 13);
    assert(std::get<bool>(mapped.at("flag")));
    assert(std::get<std::int64_t>(mapped.at("signed")) == -3);
    assert(std::get<std::uint64_t>(mapped.at("unsigned")) == 4);
    assert(std::get<double>(mapped.at("number")) == 2.5);
    assert(std::get<std::string>(mapped.at("name")) == "road");
    assert(std::get<std::vector<std::int64_t>>(mapped.at("numbers")) ==
           std::vector<std::int64_t>({1, 2}));
    assert(std::get<std::vector<bool>>(mapped.at("flags")) ==
        std::vector<bool>({true, false}));
    assert(std::get<std::vector<std::uint64_t>>(mapped.at("unsigneds")) ==
        std::vector<std::uint64_t>({3, 4}));
    assert(std::get<std::vector<double>>(mapped.at("doubles")) ==
        std::vector<double>({1.5, 2.5}));
    assert(std::get<std::vector<std::string>>(mapped.at("names")) ==
        std::vector<std::string>({"first", "second"}));
    assert(std::get<usdvector::authoring::CanonicalJson>(mapped.at("empty"))
         .text == "[]");
    assert(std::get<usdvector::authoring::CanonicalJson>(mapped.at("mixed"))
               .text == "[1,2.0]");
    assert(std::get<usdvector::authoring::CanonicalJson>(mapped.at("object"))
               .text == "{\"a\":true,\"z\":1}");
}

void TestMvpGeometryPlans() {
    const usdvector::Polygon polygon{
        {{0.0, 0.0}, {2.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}}, {}};
    const usdvector::MultiLineString multiLine{
        {usdvector::LineString{{{0.0, 0.0}, {1.0, 1.0}}},
         usdvector::LineString{{{2.0, 2.0}, {3.0, 3.0}}}}};
    const std::vector<usdvector::Feature> features = {
        {std::nullopt, usdvector::Geometry{usdvector::Point{{0.0, 0.0}}}, {},
         std::nullopt},
        {std::nullopt,
         usdvector::Geometry{usdvector::MultiPoint{{{0.0, 0.0}, {1.0, 1.0}}}},
         {}, std::nullopt},
        {std::nullopt,
         usdvector::Geometry{usdvector::LineString{{{0.0, 0.0}, {1.0, 1.0}}}},
         {}, std::nullopt},
                {std::nullopt, usdvector::Geometry{multiLine}, {}, std::nullopt},
        {std::nullopt, usdvector::Geometry{polygon}, {}, std::nullopt},
        {std::nullopt,
         usdvector::Geometry{usdvector::MultiPolygon{{polygon, polygon}}}, {},
         std::nullopt},
        {std::nullopt, usdvector::Geometry{}, {}, std::nullopt},
    };
    auto result = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, features.size()},
        features);
    assert(result.Succeeded());
    assert(result.value->features.size() == 7);
    assert(result.value->features[0].geometry.sourceType ==
           usdvector::GeometryType::Point);
    assert(result.value->features[1].geometry.points.size() == 2);
    assert(result.value->features[2].geometry.curveVertexCounts ==
           std::vector<std::uint32_t>{2});
    assert(result.value->features[3].geometry.curveVertexCounts ==
           std::vector<std::uint32_t>({2, 2}));
    assert(result.value->features[4].geometry.triangles.size() == 2);
    assert(result.value->features[5].geometry.children.size() == 2);
    assert(result.value->features[6].geometry.sourceType ==
           usdvector::GeometryType::Null);
}

void TestInvalidPolygonTopologyIsRejected() {
    const usdvector::Polygon selfIntersecting{
        {{0.0, 0.0}, {2.0, 2.0}, {0.0, 2.0}, {2.0, 0.0}}, {}};
    auto selfIntersection =
        usdvector::authoring::TriangulatePolygon(selfIntersecting);
    assert(!selfIntersection.Succeeded());
    assert(selfIntersection.diagnostics[0].code ==
           usdvector::DiagnosticCode::PolygonTriangulationFailed);

    const usdvector::Polygon outer{
        {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}}, {}};
    const usdvector::Polygon outsideHole{
        outer.outer, {{{11.0, 1.0}, {11.0, 2.0}, {12.0, 2.0}, {12.0, 1.0}}}};
    auto outside = usdvector::authoring::TriangulatePolygon(outsideHole);
    assert(!outside.Succeeded());

    const usdvector::Polygon overlappingHoles{
        outer.outer,
        {{{2.0, 2.0}, {2.0, 6.0}, {6.0, 6.0}, {6.0, 2.0}},
         {{4.0, 4.0}, {4.0, 8.0}, {8.0, 8.0}, {8.0, 4.0}}}};
    auto overlap =
        usdvector::authoring::TriangulatePolygon(overlappingHoles);
    assert(!overlap.Succeeded());
}

void TestStrictDiagnosticsAndDeclaredBounds() {
    usdvector::Feature first;
    first.id = usdvector::FeatureId{"same"};
    first.geometry = usdvector::Geometry{usdvector::Point{{0.0, 0.0}}};
    usdvector::Feature second = first;
    auto duplicate = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 2},
        {first, second}, usdvector::authoring::AuthoringOptions{true});
    assert(!duplicate.Succeeded());
    assert(duplicate.diagnostics[0].code ==
           usdvector::DiagnosticCode::DuplicateFeatureId);
    assert(duplicate.diagnostics[0].severity == usdvector::Severity::Error);

    usdvector::Bounds declared;
    declared.Include({0.0, 0.0});
    declared.Include({10.0, 10.0});
    auto datasetBounds = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, declared,
                                   std::nullopt, 1},
        {first});
    assert(datasetBounds.Succeeded());
    assert(datasetBounds.diagnostics.size() == 1);
    assert(datasetBounds.diagnostics[0].code ==
           usdvector::DiagnosticCode::BoundsMismatch);
    assert(datasetBounds.diagnostics[0].severity == usdvector::Severity::Warning);

    first.declaredBounds = declared;
    auto strictBounds = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 1},
        {first}, usdvector::authoring::AuthoringOptions{true});
    assert(!strictBounds.Succeeded());
    assert(strictBounds.diagnostics[0].code ==
           usdvector::DiagnosticCode::BoundsMismatch);
    assert(strictBounds.diagnostics[0].severity == usdvector::Severity::Error);
}

void TestDirectTriangulationRejectsNonFiniteCoordinates() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const usdvector::Polygon polygon{
        {{0.0, 0.0}, {1.0, 0.0}, {nan, 1.0}}, {}};
    auto result = usdvector::authoring::TriangulatePolygon(polygon);
    assert(!result.Succeeded());
    assert(result.diagnostics[0].code == usdvector::DiagnosticCode::InvalidCoordinate);
}

void TestMultiPolygonDiagnosticsKeepPartAnchors() {
    const usdvector::Polygon valid{
        {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 1.0}}, {}};
    const usdvector::Polygon invalid{
        {{0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}}, {}};
    auto result = usdvector::authoring::BuildAuthoringPlan(
        usdvector::DatasetMetadata{"GeoJSON", std::nullopt, std::nullopt,
                                   std::nullopt, 1},
        {{std::nullopt,
          usdvector::Geometry{usdvector::MultiPolygon{{valid, invalid}}}, {},
          std::nullopt}});
    assert(!result.Succeeded());
    assert(result.diagnostics[0].partIndex.has_value());
    assert(*result.diagnostics[0].partIndex == 1);
    assert(result.diagnostics[0].ringIndex.has_value());
    assert(*result.diagnostics[0].ringIndex == 0);
}

}  // namespace

int main() {
    TestLocalOriginPreservesSmallDifferences();
    TestPolygonHoleTriangulation();
    TestNamesAndPropertiesAreDeterministic();
    TestFeaturePlanBuilderPreservesBatchOrderAndMapping();
    TestUsdPropertyMappingUsesTypedValuesAndCanonicalFallback();
    TestMvpGeometryPlans();
    TestInvalidPolygonTopologyIsRejected();
    TestStrictDiagnosticsAndDeclaredBounds();
    TestDirectTriangulationRejectsNonFiniteCoordinates();
    TestMultiPolygonDiagnosticsKeepPartAnchors();
}

#include "usdvector/authoring/authoring.h"

#include <cassert>
#include <cmath>
#include <cstdint>
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

}  // namespace

int main() {
    TestLocalOriginPreservesSmallDifferences();
    TestPolygonHoleTriangulation();
    TestNamesAndPropertiesAreDeterministic();
    TestMvpGeometryPlans();
}

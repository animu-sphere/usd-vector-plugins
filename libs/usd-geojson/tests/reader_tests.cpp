#include "usdvector/geojson/reader.h"

#include "usdvector/core/geometry.h"

#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <variant>

namespace {

const char* Sample() {
    return R"json({
        "type": "FeatureCollection",
        "bbox": [-10, -10, 10, 10],
        "vendor": {"revision": "r3"},
        "features": [
            {"type":"Feature","id":"point","geometry":{"type":"Point","coordinates":[1,2]},"properties":{"name":"A","count":-3,"large":9223372036854775808,"nested":{"enabled":true},"values":[1,2]},"source_tag":"survey"},
            {"type":"Feature","geometry":{"type":"MultiPoint","coordinates":[[0,0],[1,1]]},"properties":null},
            {"type":"Feature","geometry":{"type":"LineString","coordinates":[[0,0],[1,1]]},"properties":{}},
            {"type":"Feature","geometry":{"type":"MultiLineString","coordinates":[[[0,0],[1,1]]]},"properties":{}},
            {"type":"Feature","geometry":{"type":"Polygon","coordinates":[[[0,0],[4,0],[4,4],[0,4],[0,0]],[[1,1],[1,2],[2,2],[1,1]]]},"properties":{}},
            {"type":"Feature","geometry":{"type":"MultiPolygon","coordinates":[[[[5,5],[6,5],[6,6],[5,5]]]]},"properties":{}},
            {"type":"Feature","geometry":null,"properties":{}}
        ]
    })json";
}

void TestFeatureCollectionAndProperties() {
    auto result = usdvector::geojson::Reader::Create(Sample());
    assert(result.Succeeded());
    auto metadata = result.value->ReadMetadata();
    assert(metadata.Succeeded());
    assert(metadata.value->format == "GeoJSON");
    assert(metadata.value->featureCount == 7);
    assert(metadata.value->declaredBounds.has_value());
    assert(metadata.value->computedBounds.has_value());
    assert(std::holds_alternative<usdvector::PropertyValue::Object>(
        metadata.value->foreignMembers.at("vendor").value));
    assert(std::get<std::string>(
               std::get<usdvector::PropertyValue::Object>(
                   metadata.value->foreignMembers.at("vendor").value)
                   .at("revision")
                   .value) == "r3");
    auto first = result.value->ReadNext();
    assert(first.Succeeded() && first.value->has_value());
    assert(usdvector::GetGeometryType(first.value->value().geometry) ==
           usdvector::GeometryType::Point);
    assert(first.value->value().id.has_value());
    assert(std::get<std::string>(*first.value->value().id) == "point");
    assert(std::get<std::int64_t>(first.value->value().properties.at("count").value) == -3);
        assert(std::get<std::uint64_t>(first.value->value().properties.at("large").value) ==
            9223372036854775808ULL);
    const auto& nested = std::get<usdvector::PropertyValue::Object>(
        first.value->value().properties.at("nested").value);
    assert(std::get<bool>(nested.at("enabled").value));
    assert(std::get<usdvector::PropertyValue::Array>(
               first.value->value().properties.at("values").value)
               .size() == 2);
    assert(std::get<std::string>(
               first.value->value().foreignMembers.at("source_tag").value) ==
           "survey");
}

void TestAllGeometryFamiliesAndEndOfStream() {
    auto result = usdvector::geojson::Reader::Create(Sample());
    assert(result.Succeeded());
    for (int index = 0; index < 7; ++index) {
        auto feature = result.value->ReadNext();
        assert(feature.Succeeded() && feature.value->has_value());
    }
    auto end = result.value->ReadNext();
    assert(end.Succeeded() && !end.value->has_value());
}

void TestLazyReaderMatchesBufferedReader() {
    auto buffered = usdvector::geojson::Reader::Create(Sample());
    auto lazy = usdvector::geojson::Reader::CreateLazy(Sample());
    assert(buffered.Succeeded() && lazy.Succeeded());

    for (int index = 0; index < 7; ++index) {
        auto bufferedFeature = buffered.value->ReadNext();
        auto lazyFeature = lazy.value->ReadNext();
        assert(bufferedFeature.Succeeded() && lazyFeature.Succeeded());
        assert(bufferedFeature.value->has_value() && lazyFeature.value->has_value());
        assert(usdvector::GetGeometryType(lazyFeature.value->value().geometry) ==
               usdvector::GetGeometryType(bufferedFeature.value->value().geometry));
        assert(lazyFeature.value->value().id == bufferedFeature.value->value().id);
        if (index == 0) {
            const auto& bufferedPoint = std::get<usdvector::Point>(
                bufferedFeature.value->value().geometry);
            const auto& lazyPoint = std::get<usdvector::Point>(
                lazyFeature.value->value().geometry);
            assert(lazyPoint.coordinate.x == bufferedPoint.coordinate.x);
            assert(lazyPoint.coordinate.y == bufferedPoint.coordinate.y);
        }
        assert(lazyFeature.value->value().properties.size() ==
               bufferedFeature.value->value().properties.size());
    }
    assert(!lazy.value->ReadNext().value->has_value());
        auto metadataAfterEnd = lazy.value->ReadMetadata();
        assert(metadataAfterEnd.Succeeded() &&
            metadataAfterEnd.value->featureCount == 7);
        auto repeatedEnd = lazy.value->ReadNext();
        assert(repeatedEnd.Succeeded() && !repeatedEnd.value->has_value());

    auto bufferedMetadata = buffered.value->ReadMetadata();
    auto lazyMetadata = lazy.value->ReadMetadata();
    assert(bufferedMetadata.Succeeded() && lazyMetadata.Succeeded());
    assert(lazyMetadata.value->format == bufferedMetadata.value->format);
    assert(lazyMetadata.value->featureCount ==
           bufferedMetadata.value->featureCount);
    assert(lazyMetadata.value->computedBounds.has_value());
    assert(bufferedMetadata.value->computedBounds.has_value());
    assert(lazyMetadata.value->computedBounds->minX ==
           bufferedMetadata.value->computedBounds->minX);
    assert(lazyMetadata.value->computedBounds->maxY ==
           bufferedMetadata.value->computedBounds->maxY);
    assert(lazyMetadata.diagnostics.size() == bufferedMetadata.diagnostics.size());
}

void TestLazyReaderEmptyFeatureCollection() {
    auto lazy = usdvector::geojson::Reader::CreateLazy(
        R"({"type":"FeatureCollection","features":[]})");
    assert(lazy.Succeeded());
    auto metadata = lazy.value->ReadMetadata();
    assert(metadata.Succeeded());
    assert(metadata.value->featureCount == 0);
    auto end = lazy.value->ReadNext();
    assert(end.Succeeded() && !end.value->has_value());
}

void TestLazyMetadataDoesNotConsumeFeatures() {
    auto lazy = usdvector::geojson::Reader::CreateLazy(Sample());
    assert(lazy.Succeeded());
    auto metadata = lazy.value->ReadMetadata();
    assert(metadata.Succeeded());
    assert(metadata.value->computedBounds.has_value());

    auto first = lazy.value->ReadNext();
    assert(first.Succeeded() && first.value->has_value());
    assert(std::get<usdvector::Point>(first.value->value().geometry)
               .coordinate.x == 1.0);
}

void TestLazyStrictBoundsFailurePersists() {
    auto lazy = usdvector::geojson::Reader::CreateLazy(
        R"({"type":"FeatureCollection","bbox":[0,0,1,1],"features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[10,10]},"properties":{}}]})",
        usdvector::geojson::ParseOptions{true});
    assert(lazy.Succeeded());
    auto feature = lazy.value->ReadNext();
    assert(feature.Succeeded() && feature.value->has_value());

    auto firstEnd = lazy.value->ReadNext();
    assert(!firstEnd.Succeeded());
    assert(!firstEnd.diagnostics.empty());
    assert(firstEnd.diagnostics[0].code == usdvector::DiagnosticCode::BoundsMismatch);

    auto repeatedEnd = lazy.value->ReadNext();
    assert(!repeatedEnd.Succeeded());
    assert(!repeatedEnd.diagnostics.empty());
    assert(repeatedEnd.diagnostics[0].code ==
           usdvector::DiagnosticCode::BoundsMismatch);
}

void TestLazyReaderRootDiagnostics() {
    auto unsupported = usdvector::geojson::Reader::CreateLazy("[]");
    assert(!unsupported.Succeeded());
    assert(unsupported.diagnostics.size() == 1);
    assert(unsupported.diagnostics[0].code ==
           usdvector::DiagnosticCode::UnsupportedGeoJsonRoot);

    auto malformed = usdvector::geojson::Reader::CreateLazy("{");
    assert(!malformed.Succeeded());
    assert(malformed.diagnostics.size() == 1);
    assert(malformed.diagnostics[0].code ==
           usdvector::DiagnosticCode::MalformedJson);

    auto invalidFeatures = usdvector::geojson::Reader::CreateLazy(
        R"({"type":"FeatureCollection","features":{}})");
    assert(!invalidFeatures.Succeeded());
    assert(invalidFeatures.diagnostics.size() == 1);
    assert(invalidFeatures.diagnostics[0].code ==
           usdvector::DiagnosticCode::InvalidFeatureCollection);

    auto invalidFeature = usdvector::geojson::Reader::CreateLazy(
        R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"GeometryCollection","geometries":[]},"properties":{}},{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0]},"properties":[]}]})");
    assert(invalidFeature.Succeeded());
    auto invalidFeatureResult = invalidFeature.value->ReadNext();
    assert(!invalidFeatureResult.Succeeded());
    assert(!invalidFeatureResult.diagnostics.empty());
    assert(invalidFeatureResult.diagnostics[0].code ==
           usdvector::DiagnosticCode::UnsupportedGeometryType);
    auto repeatedFeatureResult = invalidFeature.value->ReadNext();
    assert(!repeatedFeatureResult.Succeeded());
    assert(repeatedFeatureResult.diagnostics.size() ==
           invalidFeatureResult.diagnostics.size());
    assert(repeatedFeatureResult.diagnostics[0].code ==
           invalidFeatureResult.diagnostics[0].code);
    auto metadataAfterFailure = invalidFeature.value->ReadMetadata();
    assert(!metadataAfterFailure.Succeeded());
    assert(metadataAfterFailure.diagnostics.size() ==
           invalidFeatureResult.diagnostics.size());
    assert(metadataAfterFailure.diagnostics[0].code ==
           invalidFeatureResult.diagnostics[0].code);
}

void TestStableDiagnostics() {
    auto malformed = usdvector::geojson::Reader::Create("{");
    assert(!malformed.Succeeded());
    assert(malformed.diagnostics.size() == 1);
    assert(malformed.diagnostics[0].code == usdvector::DiagnosticCode::MalformedJson);

    auto unsupported = usdvector::geojson::Reader::Create(
        R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"GeometryCollection","geometries":[]},"properties":{}}]})");
    assert(!unsupported.Succeeded());
    assert(unsupported.diagnostics[0].code ==
           usdvector::DiagnosticCode::UnsupportedGeometryType);

    auto geometryForeignMember = usdvector::geojson::Reader::Create(
        R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0],"vendor":"x"},"properties":{}}]})");
    assert(geometryForeignMember.Succeeded());
    auto geometryForeignMemberMetadata =
        geometryForeignMember.value->ReadMetadata();
    assert(geometryForeignMemberMetadata.Succeeded());
    assert(geometryForeignMemberMetadata.diagnostics.size() == 1);
    assert(geometryForeignMemberMetadata.diagnostics[0].code ==
           usdvector::DiagnosticCode::ForeignMemberLimit);
    assert(geometryForeignMemberMetadata.diagnostics[0].severity ==
           usdvector::Severity::Warning);

    auto strictGeometryForeignMember = usdvector::geojson::Reader::Create(
        R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":{"type":"Point","coordinates":[0,0],"vendor":"x"},"properties":{}}]})",
        usdvector::geojson::ParseOptions{true});
    assert(!strictGeometryForeignMember.Succeeded());
    assert(strictGeometryForeignMember.diagnostics[0].code ==
           usdvector::DiagnosticCode::ForeignMemberLimit);
    assert(strictGeometryForeignMember.diagnostics[0].severity ==
           usdvector::Severity::Error);
}

}  // namespace

int main() {
    try {
        TestFeatureCollectionAndProperties();
        TestAllGeometryFamiliesAndEndOfStream();
        TestLazyReaderMatchesBufferedReader();
        TestLazyReaderEmptyFeatureCollection();
        TestLazyMetadataDoesNotConsumeFeatures();
        TestLazyStrictBoundsFailurePersists();
        TestLazyReaderRootDiagnostics();
        TestStableDiagnostics();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
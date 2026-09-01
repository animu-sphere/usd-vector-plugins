#include "usdvector/core/bounds.h"
#include "usdvector/core/diagnostics.h"
#include "usdvector/core/identifiers.h"
#include "usdvector/core/reader.h"
#include "usdvector/core/validation.h"

#include <cassert>
#include <cmath>
#include <limits>

namespace {

void TestDiagnosticCodes() {
    assert(std::string(usdvector::DiagnosticCodeString(
               usdvector::DiagnosticCode::InvalidCoordinate)) == "VEC002");
    assert(std::string(usdvector::DiagnosticCodeString(
               usdvector::DiagnosticCode::LocalCoordinatePrecision)) == "VEC009");
}

void TestBoundsPreserveLargeCoordinates() {
    usdvector::LineString line{{{{1000000000.125, -2000000000.5, 12.0},
                                 {1000000000.375, -2000000000.0, 18.0}}}};
    usdvector::Bounds bounds = usdvector::ComputeBounds(usdvector::Geometry{line});
    usdvector::Coordinate center = bounds.Center();
    assert(!bounds.empty);
    assert(bounds.minX == 1000000000.125);
    assert(center.x == 1000000000.25);
    assert(center.z.has_value() && *center.z == 15.0);
}

void TestEmptyBoundsUseZeroOrigin() {
    usdvector::Bounds bounds = usdvector::ComputeBounds(usdvector::Geometry{});
    usdvector::Coordinate origin = bounds.Center();
    assert(bounds.empty);
    assert(origin.x == 0.0 && origin.y == 0.0 && !origin.z.has_value());
}

void TestEmptyMultiGeometriesAreRejected() {
    const std::vector<usdvector::Geometry> geometries = {
        usdvector::MultiPoint{}, usdvector::MultiLineString{},
        usdvector::MultiPolygon{}};
    for (const usdvector::Geometry& geometry : geometries) {
        const auto diagnostics = usdvector::ValidateGeometry(geometry);
        assert(diagnostics.size() == 1);
        assert(diagnostics[0].code == usdvector::DiagnosticCode::InsufficientCoordinates);
    }
}

void TestRingNormalization() {
    usdvector::Ring ring{{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {0.0, 0.0}};
    usdvector::Result<usdvector::Ring> result = usdvector::NormalizeRing(ring);
    assert(result.Succeeded());
    assert(result.value->size() == 3);
}

void TestValidationAnchorsAndStrictDimensions() {
    usdvector::MultiLineString lines{{{{{0.0, 0.0}, {0.0, 0.0}}},
                                      {{{1.0, 1.0},
                                        {std::numeric_limits<double>::quiet_NaN(), 2.0}}}}};
    auto diagnostics = usdvector::ValidateGeometry(
        usdvector::Geometry{lines}, usdvector::ValidationOptions{true});
    assert(diagnostics.size() == 2);
    assert(diagnostics[0].code == usdvector::DiagnosticCode::InsufficientCoordinates);
    assert(diagnostics[0].partIndex.has_value() && *diagnostics[0].partIndex == 0);
    assert(diagnostics[1].code == usdvector::DiagnosticCode::InvalidCoordinate);
    assert(diagnostics[1].partIndex.has_value() && *diagnostics[1].partIndex == 1);
}

void TestStrictDimensionsSpanMultiParts() {
    usdvector::MultiLineString lines{{{{{0.0, 0.0}, {1.0, 1.0}}},
                                      {{{2.0, 2.0, 3.0}, {3.0, 3.0, 4.0}}}}};
    auto diagnostics = usdvector::ValidateGeometry(
        usdvector::Geometry{lines}, usdvector::ValidationOptions{true});
    assert(diagnostics.size() == 2);
    assert(diagnostics[0].code == usdvector::DiagnosticCode::InvalidCoordinate);
    assert(diagnostics[0].partIndex.has_value() && *diagnostics[0].partIndex == 1);
    assert(diagnostics[1].code == usdvector::DiagnosticCode::InvalidCoordinate);
}

void TestFeatureReaderContract() {
    class EmptyReader final : public usdvector::FeatureReader {
    public:
        usdvector::Result<usdvector::DatasetMetadata> ReadMetadata() override {
            return usdvector::Result<usdvector::DatasetMetadata>::Success(
                usdvector::DatasetMetadata{"test", std::nullopt, std::nullopt,
                                           std::nullopt, 0});
        }

        usdvector::Result<std::optional<usdvector::Feature>> ReadNext() override {
            return usdvector::Result<std::optional<usdvector::Feature>>::Success(
                std::nullopt);
        }
    } reader;

    assert(reader.ReadMetadata().Succeeded());
    auto next = reader.ReadNext();
    assert(next.Succeeded() && !next.value->has_value());
}

void TestDeterministicFeatureNames() {
    assert(usdvector::NormalizeIdentifier("road name") == "road_name");
    assert(usdvector::NormalizeIdentifier("7th avenue") == "_7th_avenue");
    assert(usdvector::MakeFeatureName(usdvector::FeatureId{"road 7"}) ==
           "id_road_7");
    assert(usdvector::MakeFeatureName(usdvector::FeatureId{std::int64_t{-12}}) ==
           "id__12");
    assert(usdvector::MakeFeatureName(3) == "f_00000003");
}

}  // namespace

int main() {
    TestDiagnosticCodes();
    TestDeterministicFeatureNames();
    TestBoundsPreserveLargeCoordinates();
    TestEmptyBoundsUseZeroOrigin();
    TestEmptyMultiGeometriesAreRejected();
    TestRingNormalization();
    TestValidationAnchorsAndStrictDimensions();
    TestStrictDimensionsSpanMultiParts();
    TestFeatureReaderContract();
}
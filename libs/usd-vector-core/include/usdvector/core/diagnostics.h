#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace usdvector {

enum class Severity {
    Warning,
    Error,
};

enum class DiagnosticCode {
    UnsupportedGeometryType,
    InvalidCoordinate,
    InsufficientCoordinates,
    PolygonTriangulationFailed,
    PropertyRepresentationFailed,
    DuplicateFeatureId,
    PropertyNameNormalized,
    BoundsMismatch,
    LocalCoordinatePrecision,
    UsdAuthoringFailed,
    MalformedJson,
    UnsupportedGeoJsonRoot,
    InvalidGeoJsonMember,
    LegacyGeoJsonCrs,
    InvalidFeatureCollection,
    InvalidGeoJsonProperties,
    InvalidGeoJsonBbox,
    ForeignMemberLimit,
};

const char* DiagnosticCodeString(DiagnosticCode code);

struct Diagnostic {
    DiagnosticCode code;
    Severity severity;
    std::string message;
    std::optional<std::uint64_t> byteOffset;
    std::optional<std::uint64_t> featureIndex;
    std::optional<std::string> featureId;
    std::optional<std::uint32_t> partIndex;
    std::optional<std::uint32_t> ringIndex;
    std::optional<std::uint64_t> coordinateIndex;
    std::optional<std::string> propertyName;
};

template <typename T>
struct Result {
    std::optional<T> value;
    std::vector<Diagnostic> diagnostics;

    bool Succeeded() const { return value.has_value(); }

    static Result Success(T result) {
        Result output;
        output.value = std::move(result);
        return output;
    }

    static Result Failure(std::vector<Diagnostic> errors) {
        Result output;
        output.diagnostics = std::move(errors);
        return output;
    }
};

}  // namespace usdvector
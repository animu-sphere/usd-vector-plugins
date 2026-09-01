#include "usdvector/core/diagnostics.h"

namespace usdvector {

const char* DiagnosticCodeString(DiagnosticCode code) {
    switch (code) {
        case DiagnosticCode::UnsupportedGeometryType:
            return "VEC001";
        case DiagnosticCode::InvalidCoordinate:
            return "VEC002";
        case DiagnosticCode::InsufficientCoordinates:
            return "VEC003";
        case DiagnosticCode::PolygonTriangulationFailed:
            return "VEC004";
        case DiagnosticCode::PropertyRepresentationFailed:
            return "VEC005";
        case DiagnosticCode::DuplicateFeatureId:
            return "VEC006";
        case DiagnosticCode::PropertyNameNormalized:
            return "VEC007";
        case DiagnosticCode::BoundsMismatch:
            return "VEC008";
        case DiagnosticCode::LocalCoordinatePrecision:
            return "VEC009";
        case DiagnosticCode::UsdAuthoringFailed:
            return "VEC010";
        case DiagnosticCode::MalformedJson:
            return "GJSON001";
        case DiagnosticCode::UnsupportedGeoJsonRoot:
            return "GJSON002";
        case DiagnosticCode::InvalidGeoJsonMember:
            return "GJSON003";
        case DiagnosticCode::LegacyGeoJsonCrs:
            return "GJSON004";
        case DiagnosticCode::InvalidFeatureCollection:
            return "GJSON005";
        case DiagnosticCode::InvalidGeoJsonProperties:
            return "GJSON006";
        case DiagnosticCode::InvalidGeoJsonBbox:
            return "GJSON007";
        case DiagnosticCode::ForeignMemberLimit:
            return "GJSON008";
    }
    return "VEC001";
}

}  // namespace usdvector
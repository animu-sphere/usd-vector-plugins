#include "usdvector/geojson/reader.h"

#include "usdvector/core/bounds.h"
#include "usdvector/core/validation.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <utility>

namespace usdvector::geojson {
namespace {

using Json = nlohmann::json;

Diagnostic Issue(DiagnosticCode code, Severity severity, std::string message,
                 std::optional<std::uint64_t> featureIndex = std::nullopt) {
    return {code, severity, std::move(message), std::nullopt, featureIndex,
            std::nullopt, std::nullopt, std::nullopt, std::nullopt,
            std::nullopt};
}

void AddUnknownMemberDiagnostics(
    const Json& object, std::initializer_list<const char*> knownMembers,
    const ParseOptions& options, std::vector<Diagnostic>& diagnostics,
    std::optional<std::uint64_t> featureIndex = std::nullopt) {
    for (const auto& item : object.items()) {
        bool known = false;
        for (const char* member : knownMembers) {
            if (item.key() == member) {
                known = true;
                break;
            }
        }
        if (!known) {
            diagnostics.push_back(Issue(
                DiagnosticCode::ForeignMemberLimit,
                options.strict ? Severity::Error : Severity::Warning,
                "geometry foreign member cannot be preserved by the MVP model",
                featureIndex));
        }
    }
}

bool HasErrors(const std::vector<Diagnostic>& diagnostics) {
    for (const Diagnostic& diagnostic : diagnostics) {
        if (diagnostic.severity == Severity::Error) {
            return true;
        }
    }
    return false;
}

bool IsFiniteNumber(const Json& value) {
    return value.is_number() && std::isfinite(value.get<double>());
}

bool ParseCoordinate(const Json& value, Coordinate& coordinate,
                     std::vector<Diagnostic>& diagnostics) {
    if (!value.is_array() || (value.size() != 2 && value.size() != 3) ||
        !IsFiniteNumber(value[0]) || !IsFiniteNumber(value[1]) ||
        (value.size() == 3 && !IsFiniteNumber(value[2]))) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
            "position must contain two or three finite numbers"));
        return false;
    }
    coordinate.x = value[0].get<double>();
    coordinate.y = value[1].get<double>();
    coordinate.z = value.size() == 3
                       ? std::optional<double>{value[2].get<double>()}
                       : std::nullopt;
    return true;
}

bool ParseCoordinateList(const Json& value, std::vector<Coordinate>& coordinates,
                         std::vector<Diagnostic>& diagnostics) {
    if (!value.is_array()) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
            "coordinates member must be an array"));
        return false;
    }
    bool valid = true;
    for (const Json& item : value) {
        Coordinate coordinate;
        if (ParseCoordinate(item, coordinate, diagnostics)) {
            coordinates.push_back(coordinate);
        } else {
            valid = false;
        }
    }
    return valid;
}

bool ParseRing(const Json& value, Ring& ring,
               std::vector<Diagnostic>& diagnostics) {
    std::vector<Coordinate> coordinates;
    if (!ParseCoordinateList(value, coordinates, diagnostics)) {
        return false;
    }
    Result<Ring> normalized = NormalizeRing(coordinates);
    if (!normalized.Succeeded()) {
        diagnostics.insert(diagnostics.end(), normalized.diagnostics.begin(),
                           normalized.diagnostics.end());
        return false;
    }
    ring = std::move(*normalized.value);
    return true;
}

bool ParseBounds(const Json& value, Bounds& bounds,
                 std::vector<Diagnostic>& diagnostics) {
    if (!value.is_array() || (value.size() != 4 && value.size() != 6)) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonBbox, Severity::Error,
            "bbox must contain four or six numbers"));
        return false;
    }
    for (const Json& item : value) {
        if (!IsFiniteNumber(item)) {
            diagnostics.push_back(Issue(
                DiagnosticCode::InvalidGeoJsonBbox, Severity::Error,
                "bbox must contain only finite numbers"));
            return false;
        }
    }
    bounds.empty = false;
    bounds.minX = value[0].get<double>();
    bounds.minY = value[1].get<double>();
    bounds.maxX = value[2].get<double>();
    bounds.maxY = value[3].get<double>();
    if (value.size() == 6) {
        bounds.minZ = value[4].get<double>();
        bounds.maxZ = value[5].get<double>();
    }
    if (bounds.minX > bounds.maxX || bounds.minY > bounds.maxY ||
        (bounds.minZ.has_value() && *bounds.minZ > *bounds.maxZ)) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonBbox, Severity::Error,
            "bbox minimum must not exceed its maximum"));
        return false;
    }
    return true;
}

bool BoundsEqual(const Bounds& left, const Bounds& right) {
    return left.empty == right.empty &&
           (left.empty || (left.minX == right.minX && left.minY == right.minY &&
                           left.maxX == right.maxX && left.maxY == right.maxY &&
                           left.minZ == right.minZ && left.maxZ == right.maxZ));
}

bool ParseProperty(const Json& value, PropertyValue& property,
                   std::vector<Diagnostic>& diagnostics,
                   std::optional<std::uint64_t> featureIndex) {
    if (value.is_null()) {
        property = PropertyValue{};
    } else if (value.is_boolean()) {
        property = PropertyValue{value.get<bool>()};
    } else if (value.is_number_unsigned()) {
        property = PropertyValue{value.get<std::uint64_t>()};
    } else if (value.is_number_integer()) {
        property = PropertyValue{value.get<std::int64_t>()};
    } else if (value.is_number_float()) {
        property = PropertyValue{value.get<double>()};
    } else if (value.is_string()) {
        property = PropertyValue{value.get<std::string>()};
    } else if (value.is_array()) {
        PropertyValue::Array array;
        for (const Json& item : value) {
            PropertyValue nested;
            if (!ParseProperty(item, nested, diagnostics, featureIndex)) {
                return false;
            }
            array.push_back(std::move(nested));
        }
        property = PropertyValue{std::move(array)};
    } else if (value.is_object()) {
        PropertyValue::Object object;
        for (const auto& item : value.items()) {
            PropertyValue nested;
            if (!ParseProperty(item.value(), nested, diagnostics, featureIndex)) {
                return false;
            }
            object.emplace(item.key(), std::move(nested));
        }
        property = PropertyValue{std::move(object)};
    } else {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonProperties, Severity::Error,
            "property contains an unsupported JSON value", featureIndex));
        return false;
    }
    return true;
}

bool ParseForeignMembers(const Json& object,
                         std::initializer_list<const char*> knownMembers,
                         ForeignMembers& foreignMembers,
                         std::vector<Diagnostic>& diagnostics,
                         std::optional<std::uint64_t> featureIndex) {
    for (const auto& item : object.items()) {
        bool known = false;
        for (const char* member : knownMembers) {
            if (item.key() == member) {
                known = true;
                break;
            }
        }
        if (known) {
            continue;
        }
        PropertyValue property;
        if (ParseProperty(item.value(), property, diagnostics, featureIndex)) {
            foreignMembers.emplace(item.key(), std::move(property));
        }
    }
    return true;
}

bool ParseProperties(const Json& value, Properties& properties,
                     std::vector<Diagnostic>& diagnostics,
                     std::optional<std::uint64_t> featureIndex) {
    if (value.is_null()) {
        return true;
    }
    if (!value.is_object()) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonProperties, Severity::Error,
            "feature properties must be an object or null", featureIndex));
        return false;
    }
    for (const auto& item : value.items()) {
        PropertyValue property;
        if (ParseProperty(item.value(), property, diagnostics, featureIndex)) {
            properties.emplace(item.key(), std::move(property));
        }
    }
    return true;
}

bool ParseGeometry(const Json& value, Geometry& geometry,
                   const ParseOptions& options,
                   std::vector<Diagnostic>& diagnostics) {
    if (value.is_null()) {
        geometry = std::monostate{};
        return true;
    }
    if (!value.is_object() || !value.contains("type") ||
        !value["type"].is_string()) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
            "geometry must be null or an object with a string type"));
        return false;
    }
    const std::string type = value["type"].get<std::string>();
    AddUnknownMemberDiagnostics(value,
                                {"type", "coordinates", "bbox", "geometries"},
                                options, diagnostics);
    if (type == "GeometryCollection") {
        diagnostics.push_back(Issue(
            DiagnosticCode::UnsupportedGeometryType, Severity::Error,
            "GeometryCollection is outside the MVP"));
        return false;
    }
    if (!value.contains("coordinates")) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
            "geometry is missing coordinates"));
        return false;
    }
    const Json& coordinates = value["coordinates"];
    if (type == "Point") {
        Point point;
        if (!ParseCoordinate(coordinates, point.coordinate, diagnostics)) {
            return false;
        }
        geometry = point;
    } else if (type == "MultiPoint") {
        MultiPoint points;
        if (!ParseCoordinateList(coordinates, points.coordinates, diagnostics)) {
            return false;
        }
        geometry = std::move(points);
    } else if (type == "LineString") {
        LineString line;
        if (!ParseCoordinateList(coordinates, line.coordinates, diagnostics)) {
            return false;
        }
        geometry = std::move(line);
    } else if (type == "MultiLineString") {
        if (!coordinates.is_array()) {
            diagnostics.push_back(Issue(
                DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
                "MultiLineString coordinates must be an array"));
            return false;
        }
        MultiLineString lines;
        for (const Json& lineValue : coordinates) {
            LineString line;
            if (!ParseCoordinateList(lineValue, line.coordinates, diagnostics)) {
                return false;
            }
            lines.lines.push_back(std::move(line));
        }
        geometry = std::move(lines);
    } else if (type == "Polygon") {
        if (!coordinates.is_array()) {
            diagnostics.push_back(Issue(
                DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
                "Polygon coordinates must be an array of rings"));
            return false;
        }
        Polygon polygon;
        for (std::size_t index = 0; index < coordinates.size(); ++index) {
            Ring ring;
            if (!ParseRing(coordinates[index], ring, diagnostics)) {
                return false;
            }
            if (index == 0) {
                polygon.outer = std::move(ring);
            } else {
                polygon.holes.push_back(std::move(ring));
            }
        }
        geometry = std::move(polygon);
    } else if (type == "MultiPolygon") {
        if (!coordinates.is_array()) {
            diagnostics.push_back(Issue(
                DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
                "MultiPolygon coordinates must be an array of polygons"));
            return false;
        }
        MultiPolygon polygons;
        for (const Json& polygonValue : coordinates) {
            if (!polygonValue.is_array()) {
                diagnostics.push_back(Issue(
                    DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
                    "MultiPolygon member must be an array of rings"));
                return false;
            }
            Polygon polygon;
            for (std::size_t index = 0; index < polygonValue.size(); ++index) {
                Ring ring;
                if (!ParseRing(polygonValue[index], ring, diagnostics)) {
                    return false;
                }
                if (index == 0) {
                    polygon.outer = std::move(ring);
                } else {
                    polygon.holes.push_back(std::move(ring));
                }
            }
            polygons.polygons.push_back(std::move(polygon));
        }
        geometry = std::move(polygons);
    } else {
        diagnostics.push_back(Issue(
            DiagnosticCode::UnsupportedGeometryType, Severity::Error,
            "geometry type is outside the MVP"));
        return false;
    }
    return true;
}

Feature ParseFeature(const Json& sourceFeature, std::size_t index,
                     const ParseOptions& options,
                     std::vector<Diagnostic>& diagnostics) {
    Feature feature;
    const auto featureIndex = static_cast<std::uint64_t>(index);
    const std::size_t diagnosticStart = diagnostics.size();
    if (!sourceFeature.is_object() || !sourceFeature.contains("type") ||
        !sourceFeature["type"].is_string() ||
        sourceFeature["type"].get<std::string>() != "Feature") {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
            "FeatureCollection member must be a Feature object", featureIndex));
        return feature;
    }
    ParseForeignMembers(sourceFeature,
                        {"type", "id", "geometry", "properties", "bbox"},
                        feature.foreignMembers, diagnostics, featureIndex);
    if (sourceFeature.contains("id") && !sourceFeature["id"].is_null()) {
        const Json& id = sourceFeature["id"];
        if (id.is_string()) {
            feature.id = FeatureId{id.get<std::string>()};
        } else if (id.is_number_unsigned()) {
            feature.id = FeatureId{id.get<std::uint64_t>()};
        } else if (id.is_number_integer()) {
            feature.id = FeatureId{id.get<std::int64_t>()};
        } else {
            diagnostics.push_back(Issue(
                DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
                "feature id must be a string or integer", featureIndex));
        }
    }
    if (!sourceFeature.contains("geometry")) {
        diagnostics.push_back(Issue(
            DiagnosticCode::InvalidGeoJsonMember, Severity::Error,
            "Feature is missing geometry", featureIndex));
    } else {
        ParseGeometry(sourceFeature["geometry"], feature.geometry, options,
                      diagnostics);
    }
    if (sourceFeature.contains("properties")) {
        ParseProperties(sourceFeature["properties"], feature.properties,
                        diagnostics, featureIndex);
    }
    if (sourceFeature.contains("bbox")) {
        Bounds bounds;
        if (ParseBounds(sourceFeature["bbox"], bounds, diagnostics)) {
            feature.declaredBounds = bounds;
        }
    }
    std::vector<Diagnostic> geometryDiagnostics =
        ValidateGeometry(feature.geometry, ValidationOptions{options.strict});
    diagnostics.insert(diagnostics.end(), geometryDiagnostics.begin(),
                       geometryDiagnostics.end());
    for (std::size_t diagnostic = diagnosticStart; diagnostic < diagnostics.size();
         ++diagnostic) {
        if (!diagnostics[diagnostic].featureIndex.has_value()) {
            diagnostics[diagnostic].featureIndex = featureIndex;
        }
    }
    return feature;
}

void IncludeBounds(Bounds& target, const Bounds& source) {
    if (source.empty) {
        return;
    }
    if (target.empty) {
        target = source;
        return;
    }
    target.minX = std::min(target.minX, source.minX);
    target.minY = std::min(target.minY, source.minY);
    target.maxX = std::max(target.maxX, source.maxX);
    target.maxY = std::max(target.maxY, source.maxY);
    if (source.minZ.has_value()) {
        if (!target.minZ.has_value()) {
            target.minZ = source.minZ;
            target.maxZ = source.maxZ;
        } else {
            target.minZ = std::min(*target.minZ, *source.minZ);
            target.maxZ = std::max(*target.maxZ, *source.maxZ);
        }
    }
}

struct ParsedDocument {
    DatasetMetadata metadata;
    std::vector<Feature> features;
    std::vector<Diagnostic> diagnostics;
    std::shared_ptr<Json> root;
};

Result<ParsedDocument> ParseDocument(std::string_view source,
                                      const ParseOptions& options,
                                      bool retainFeatures = true) {
    Json root;
    try {
        root = Json::parse(source.begin(), source.end());
    } catch (const std::exception& error) {
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::MalformedJson, Severity::Error,
            std::string("malformed JSON: ") + error.what())});
    }
    auto documentRoot = std::make_shared<Json>(std::move(root));
    const Json& rootDocument = *documentRoot;

    ParsedDocument document;
    document.metadata.format = "GeoJSON";
    if (!rootDocument.is_object() || !rootDocument.contains("type") ||
        !rootDocument["type"].is_string()) {
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::UnsupportedGeoJsonRoot, Severity::Error,
            "root must be a GeoJSON object with a type")});
    }
    if (rootDocument["type"].get<std::string>() != "FeatureCollection") {
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::UnsupportedGeoJsonRoot, Severity::Error,
            "root type must be FeatureCollection")});
    }
    ParseForeignMembers(rootDocument, {"type", "features", "bbox", "crs"},
                        document.metadata.foreignMembers, document.diagnostics,
                        std::nullopt);
    if (!rootDocument.contains("features") ||
        !rootDocument["features"].is_array()) {
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::InvalidFeatureCollection, Severity::Error,
            "FeatureCollection.features must be an array")});
    }
    if (rootDocument.contains("bbox")) {
        Bounds bounds;
        if (!ParseBounds(rootDocument["bbox"], bounds, document.diagnostics)) {
            return Result<ParsedDocument>::Failure(document.diagnostics);
        }
        document.metadata.declaredBounds = bounds;
    }
    if (rootDocument.contains("crs")) {
        document.metadata.crs = rootDocument["crs"].dump();
        document.diagnostics.push_back(Issue(
            DiagnosticCode::LegacyGeoJsonCrs, Severity::Warning,
            "legacy crs member was preserved as an extension"));
    }

    Bounds computedBounds;
    for (std::size_t index = 0; index < rootDocument["features"].size(); ++index) {
        Feature feature = ParseFeature(rootDocument["features"][index], index, options,
                                       document.diagnostics);
        IncludeBounds(computedBounds, ComputeBounds(feature.geometry));
        if (retainFeatures) {
            document.features.push_back(std::move(feature));
        }
    }

    document.metadata.featureCount = rootDocument["features"].size();
    document.metadata.computedBounds = computedBounds;
    if (document.metadata.declaredBounds.has_value() &&
        !BoundsEqual(*document.metadata.declaredBounds, computedBounds)) {
        document.diagnostics.push_back(Issue(
            DiagnosticCode::BoundsMismatch,
            options.strict ? Severity::Error : Severity::Warning,
            "declared bounds disagree with computed bounds"));
    }
    if (HasErrors(document.diagnostics)) {
        return Result<ParsedDocument>::Failure(document.diagnostics);
    }
    document.root = std::move(documentRoot);
    Result<ParsedDocument> result;
    result.value = std::move(document);
    result.diagnostics = result.value->diagnostics;
    return result;
}

}  // namespace

Result<DatasetMetadata> ParseMetadata(std::string_view source,
                                      const ParseOptions& options) {
    Result<ParsedDocument> parsed = ParseDocument(source, options);
    if (!parsed.Succeeded()) {
        return Result<DatasetMetadata>::Failure(std::move(parsed.diagnostics));
    }
    Result<DatasetMetadata> result;
    result.value = std::move(parsed.value->metadata);
    result.diagnostics = std::move(parsed.diagnostics);
    return result;
}

Reader::Reader(DatasetMetadata metadata, std::vector<Feature> features,
           std::vector<Diagnostic> diagnostics,
           std::shared_ptr<const void> document, ParseOptions options)
    : metadata_(std::move(metadata)),
      features_(std::move(features)),
    diagnostics_(std::move(diagnostics)),
    document_(std::move(document)),
    options_(options) {}

Result<Reader> Reader::Create(std::string source, const ParseOptions& options) {
    Result<ParsedDocument> parsed = ParseDocument(source, options);
    if (!parsed.Succeeded()) {
        return Result<Reader>::Failure(std::move(parsed.diagnostics));
    }
    return Result<Reader>::Success(Reader(std::move(parsed.value->metadata),
                                          std::move(parsed.value->features),
                                          std::move(parsed.diagnostics)));
}

Result<Reader> Reader::CreateLazy(std::string source,
                                  const ParseOptions& options) {
    Result<ParsedDocument> parsed = ParseDocument(source, options, false);
    if (!parsed.Succeeded()) {
        return Result<Reader>::Failure(std::move(parsed.diagnostics));
    }
    std::shared_ptr<const void> document = std::move(parsed.value->root);
    return Result<Reader>::Success(Reader(
        std::move(parsed.value->metadata), {}, std::move(parsed.diagnostics),
        std::move(document), options));
}

Result<DatasetMetadata> Reader::ReadMetadata() {
    Result<DatasetMetadata> result;
    result.value = metadata_;
    result.diagnostics = diagnostics_;
    return result;
}

Result<std::optional<Feature>> Reader::ReadNext() {
    if (document_) {
        const auto document = std::static_pointer_cast<const Json>(document_);
        const Json& features = (*document)["features"];
        if (nextFeature_ == features.size()) {
            return Result<std::optional<Feature>>::Success(std::nullopt);
        }
        std::vector<Diagnostic> diagnostics;
        Feature feature = ParseFeature(features[nextFeature_], nextFeature_, options_,
                                       diagnostics);
        if (HasErrors(diagnostics)) {
            return Result<std::optional<Feature>>::Failure(
                std::move(diagnostics));
        }
        ++nextFeature_;
        return Result<std::optional<Feature>>::Success(
            std::optional<Feature>{std::move(feature)});
    }
    if (nextFeature_ == features_.size()) {
        return Result<std::optional<Feature>>::Success(std::nullopt);
    }
    return Result<std::optional<Feature>>::Success(
        std::move(features_[nextFeature_++]));
}

}  // namespace usdvector::geojson
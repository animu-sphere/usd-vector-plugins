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

using SourceSpan = std::pair<std::size_t, std::size_t>;

struct ScannedRoot {
    std::vector<std::pair<std::string, SourceSpan>> members;
    bool hasFeatures = false;
    bool featuresIsArray = false;
    SourceSpan featuresSpan;
    std::size_t featureCount = 0;
};

class JsonSpanScanner {
public:
    explicit JsonSpanScanner(std::string_view source) : source_(source) {}

    bool ScanRoot(ScannedRoot& root) {
        SkipWhitespace();
        if (!Consume('{')) {
            return false;
        }
        SkipWhitespace();
        if (Consume('}')) {
            SkipWhitespace();
            return position_ == source_.size();
        }
        while (position_ < source_.size()) {
            SourceSpan keySpan;
            if (!ParseString(keySpan)) {
                return false;
            }
            std::string key;
            try {
                key = Json::parse(source_.begin() + keySpan.first,
                                  source_.begin() + keySpan.second)
                          .get<std::string>();
            } catch (const std::exception&) {
                return false;
            }
            SkipWhitespace();
            if (!Consume(':')) {
                return false;
            }
            SkipWhitespace();
            if (key == "features") {
                root.hasFeatures = true;
                root.featuresSpan.first = position_;
                if (position_ < source_.size() && source_[position_] == '[') {
                    if (!ParseArray(nullptr, &root.featureCount)) {
                        return false;
                    }
                    root.featuresIsArray = true;
                } else if (!ParseValue(root.featuresSpan)) {
                    return false;
                }
                root.featuresSpan.second = position_;
            } else {
                SourceSpan valueSpan;
                if (!ParseValue(valueSpan)) {
                    return false;
                }
                root.members.emplace_back(std::move(key), valueSpan);
            }
            SkipWhitespace();
            if (Consume('}')) {
                SkipWhitespace();
                return position_ == source_.size();
            }
            if (!Consume(',')) {
                return false;
            }
            SkipWhitespace();
        }
        return false;
    }

    template <typename Callback>
    bool ScanArrayElements(const SourceSpan& arraySpan, Callback&& callback) {
        position_ = arraySpan.first;
        if (!Consume('[')) {
            return false;
        }
        SkipWhitespace();
        if (Consume(']')) {
            return position_ == arraySpan.second;
        }
        while (position_ < source_.size()) {
            SourceSpan value;
            if (!ParseValue(value) || !callback(value)) {
                return false;
            }
            SkipWhitespace();
            if (Consume(']')) {
                return position_ == arraySpan.second;
            }
            if (!Consume(',')) {
                return false;
            }
            SkipWhitespace();
        }
        return false;
    }

    bool ScanNextArrayElement(const SourceSpan& arraySpan,
                              std::size_t& position, SourceSpan& element) {
        position_ = position;
        SkipWhitespace();
        if (position_ >= arraySpan.second || source_[position_] == ']') {
            return false;
        }
        if (!ParseValue(element)) {
            return false;
        }
        SkipWhitespace();
        if (position_ < arraySpan.second && source_[position_] == ',') {
            ++position_;
        } else if (position_ >= arraySpan.second ||
                   source_[position_] != ']') {
            return false;
        }
        position = position_;
        return true;
    }

private:
    static bool IsDigit(char value) {
        return value >= '0' && value <= '9';
    }

    static bool IsHexDigit(char value) {
        return (value >= '0' && value <= '9') ||
               (value >= 'a' && value <= 'f') ||
               (value >= 'A' && value <= 'F');
    }

    void SkipWhitespace() {
        while (position_ < source_.size()) {
            const char value = source_[position_];
            if (value != ' ' && value != '\t' && value != '\n' && value != '\r') {
                break;
            }
            ++position_;
        }
    }

    bool Consume(char expected) {
        if (position_ >= source_.size() || source_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
    }

    bool ParseString(SourceSpan& span) {
        const std::size_t begin = position_;
        if (!Consume('"')) {
            return false;
        }
        while (position_ < source_.size()) {
            const unsigned char value =
                static_cast<unsigned char>(source_[position_++]);
            if (value == '"') {
                span = {begin, position_};
                return true;
            }
            if (value < 0x20) {
                return false;
            }
            if (value != '\\') {
                continue;
            }
            if (position_ >= source_.size()) {
                return false;
            }
            const char escape = source_[position_++];
            if (escape == 'u') {
                for (int digit = 0; digit < 4; ++digit) {
                    if (position_ >= source_.size() ||
                        !IsHexDigit(source_[position_++])) {
                        return false;
                    }
                }
            } else if (escape != '"' && escape != '\\' && escape != '/' &&
                       escape != 'b' && escape != 'f' && escape != 'n' &&
                       escape != 'r' && escape != 't') {
                return false;
            }
        }
        return false;
    }

    bool ParseNumber() {
        if (position_ < source_.size() && source_[position_] == '-') {
            ++position_;
        }
        if (position_ >= source_.size()) {
            return false;
        }
        if (source_[position_] == '0') {
            ++position_;
        } else {
            if (source_[position_] < '1' || source_[position_] > '9') {
                return false;
            }
            while (position_ < source_.size() && IsDigit(source_[position_])) {
                ++position_;
            }
        }
        if (position_ < source_.size() && source_[position_] == '.') {
            ++position_;
            if (position_ >= source_.size() || !IsDigit(source_[position_])) {
                return false;
            }
            while (position_ < source_.size() && IsDigit(source_[position_])) {
                ++position_;
            }
        }
        if (position_ < source_.size() &&
            (source_[position_] == 'e' || source_[position_] == 'E')) {
            ++position_;
            if (position_ < source_.size() &&
                (source_[position_] == '+' || source_[position_] == '-')) {
                ++position_;
            }
            if (position_ >= source_.size() || !IsDigit(source_[position_])) {
                return false;
            }
            while (position_ < source_.size() && IsDigit(source_[position_])) {
                ++position_;
            }
        }
        return true;
    }

    bool ParseLiteral(std::string_view literal) {
        if (source_.substr(position_, literal.size()) != literal) {
            return false;
        }
        position_ += literal.size();
        return true;
    }

    bool ParseValue(SourceSpan& span) {
        SkipWhitespace();
        const std::size_t begin = position_;
        if (position_ >= source_.size()) {
            return false;
        }
        const char value = source_[position_];
        bool valid = false;
        if (value == '"') {
            valid = ParseString(span);
        } else if (value == '{') {
            valid = ParseObject();
        } else if (value == '[') {
            valid = ParseArray();
        } else if (value == 't') {
            valid = ParseLiteral("true");
        } else if (value == 'f') {
            valid = ParseLiteral("false");
        } else if (value == 'n') {
            valid = ParseLiteral("null");
        } else if (value == '-' || IsDigit(value)) {
            valid = ParseNumber();
        }
        if (valid) {
            span = {begin, position_};
        }
        return valid;
    }

    bool ParseObject() {
        if (!Consume('{')) {
            return false;
        }
        SkipWhitespace();
        if (Consume('}')) {
            return true;
        }
        while (position_ < source_.size()) {
            SourceSpan key;
            if (!ParseString(key)) {
                return false;
            }
            SkipWhitespace();
            if (!Consume(':')) {
                return false;
            }
            SourceSpan value;
            if (!ParseValue(value)) {
                return false;
            }
            SkipWhitespace();
            if (Consume('}')) {
                return true;
            }
            if (!Consume(',')) {
                return false;
            }
            SkipWhitespace();
        }
        return false;
    }

    bool ParseArray(std::vector<SourceSpan>* elements = nullptr,
                    std::size_t* elementCount = nullptr) {
        if (!Consume('[')) {
            return false;
        }
        SkipWhitespace();
        if (Consume(']')) {
            return true;
        }
        while (position_ < source_.size()) {
            SourceSpan value;
            if (!ParseValue(value)) {
                return false;
            }
            if (elements != nullptr) {
                elements->push_back(value);
            }
            if (elementCount != nullptr) {
                ++*elementCount;
            }
            SkipWhitespace();
            if (Consume(']')) {
                return true;
            }
            if (!Consume(',')) {
                return false;
            }
            SkipWhitespace();
        }
        return false;
    }

    std::string_view source_;
    std::size_t position_ = 0;
};

Json ParseSpan(std::string_view source, const SourceSpan& span) {
    return Json::parse(source.begin() + span.first,
                       source.begin() + span.second);
}

struct ParsedDocument {
    DatasetMetadata metadata;
    std::vector<Feature> features;
    std::vector<Diagnostic> diagnostics;
    std::shared_ptr<Json> root;
    SourceSpan featureArraySpan;
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

Result<ParsedDocument> ParseLazyDocument(std::string_view source,
                                         const ParseOptions& options) {
    ScannedRoot scanned;
    if (!JsonSpanScanner(source).ScanRoot(scanned)) {
        bool validJson = false;
        try {
            validJson = Json::accept(source.begin(), source.end());
        } catch (const std::exception&) {
            validJson = false;
        }
        const std::size_t firstValue = source.find_first_not_of(" \t\n\r");
        if (validJson &&
            (firstValue == std::string_view::npos || source[firstValue] != '{')) {
            return Result<ParsedDocument>::Failure({Issue(
                DiagnosticCode::UnsupportedGeoJsonRoot, Severity::Error,
                "root must be a GeoJSON object with a type")});
        }
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::MalformedJson, Severity::Error,
            "malformed JSON")});
    }

    ParsedDocument document;
    document.metadata.format = "GeoJSON";
    const auto findMember = [&scanned](std::string_view name)
        -> const SourceSpan* {
        for (auto iterator = scanned.members.rbegin();
             iterator != scanned.members.rend(); ++iterator) {
            if (iterator->first == name) {
                return &iterator->second;
            }
        }
        return nullptr;
    };

    const SourceSpan* typeSpan = findMember("type");
    if (typeSpan == nullptr) {
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::UnsupportedGeoJsonRoot, Severity::Error,
            "root must be a GeoJSON object with a type")});
    }
    try {
        const Json type = ParseSpan(source, *typeSpan);
        if (!type.is_string() || type.get<std::string>() != "FeatureCollection") {
            return Result<ParsedDocument>::Failure({Issue(
                DiagnosticCode::UnsupportedGeoJsonRoot, Severity::Error,
                "root type must be FeatureCollection")});
        }
    } catch (const std::exception& error) {
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::MalformedJson, Severity::Error,
            std::string("malformed JSON: ") + error.what())});
    }

    if (!scanned.hasFeatures || !scanned.featuresIsArray) {
        return Result<ParsedDocument>::Failure({Issue(
            DiagnosticCode::InvalidFeatureCollection, Severity::Error,
            "FeatureCollection.features must be an array")});
    }

    for (const auto& member : scanned.members) {
        if (member.first == "type" || member.first == "bbox" ||
            member.first == "crs") {
            continue;
        }
        try {
            PropertyValue property;
            std::vector<Diagnostic> diagnostics;
            if (ParseProperty(ParseSpan(source, member.second), property,
                              diagnostics, std::nullopt)) {
                document.metadata.foreignMembers[member.first] =
                    std::move(property);
            }
            document.diagnostics.insert(document.diagnostics.end(),
                                        diagnostics.begin(), diagnostics.end());
        } catch (const std::exception& error) {
            return Result<ParsedDocument>::Failure({Issue(
                DiagnosticCode::MalformedJson, Severity::Error,
                std::string("malformed JSON: ") + error.what())});
        }
    }

    if (const SourceSpan* bboxSpan = findMember("bbox"); bboxSpan != nullptr) {
        try {
            Bounds bounds;
            if (!ParseBounds(ParseSpan(source, *bboxSpan), bounds,
                             document.diagnostics)) {
                return Result<ParsedDocument>::Failure(document.diagnostics);
            }
            document.metadata.declaredBounds = bounds;
        } catch (const std::exception& error) {
            return Result<ParsedDocument>::Failure({Issue(
                DiagnosticCode::MalformedJson, Severity::Error,
                std::string("malformed JSON: ") + error.what())});
        }
    }
    if (const SourceSpan* crsSpan = findMember("crs"); crsSpan != nullptr) {
        try {
            document.metadata.crs = ParseSpan(source, *crsSpan).dump();
            document.diagnostics.push_back(Issue(
                DiagnosticCode::LegacyGeoJsonCrs, Severity::Warning,
                "legacy crs member was preserved as an extension"));
        } catch (const std::exception& error) {
            return Result<ParsedDocument>::Failure({Issue(
                DiagnosticCode::MalformedJson, Severity::Error,
                std::string("malformed JSON: ") + error.what())});
        }
    }

    document.metadata.featureCount = scanned.featureCount;
    document.featureArraySpan = scanned.featuresSpan;
    Result<ParsedDocument> result;
    result.value = std::move(document);
    result.diagnostics = result.value->diagnostics;
    return result;
}

}  // namespace

Result<DatasetMetadata> ParseMetadata(std::string_view source,
                                      const ParseOptions& options) {
    Result<ParsedDocument> parsed = ParseDocument(source, options, false);
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
           std::shared_ptr<const std::string> source,
           std::pair<std::size_t, std::size_t> featureArraySpan,
           std::size_t featureCount, ParseOptions options)
    : metadata_(std::move(metadata)),
      features_(std::move(features)),
            diagnostics_(std::move(diagnostics)),
            source_(std::move(source)),
            featureArraySpan_(featureArraySpan),
            featureCount_(featureCount),
            nextFeaturePosition_(featureArraySpan.first + 1),
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
    auto sourceStorage =
        std::make_shared<const std::string>(std::move(source));
    Result<ParsedDocument> parsed = ParseLazyDocument(*sourceStorage, options);
    if (!parsed.Succeeded()) {
        return Result<Reader>::Failure(std::move(parsed.diagnostics));
    }
    const std::size_t featureCount =
        parsed.value->metadata.featureCount.value_or(0);
    return Result<Reader>::Success(Reader(
        std::move(parsed.value->metadata), {}, std::move(parsed.diagnostics),
        std::move(sourceStorage), parsed.value->featureArraySpan,
        featureCount, options));
}

Result<DatasetMetadata> Reader::ReadMetadata() {
    if (source_ && lazyReadFailed_) {
        return Result<DatasetMetadata>::Failure(lazyReadFailureDiagnostics_);
    }
    if (source_ && !CompleteLazyMetadata()) {
        return Result<DatasetMetadata>::Failure(diagnostics_);
    }
    Result<DatasetMetadata> result;
    result.value = metadata_;
    result.diagnostics = diagnostics_;
    return result;
}

bool Reader::CompleteLazyMetadata() {
    if (!source_ || lazyMetadataComplete_) {
        return !lazyMetadataFailed_;
    }
    if (lazyMetadataFailed_) {
        return false;
    }

    Bounds computedBounds;
    std::vector<Diagnostic> featureDiagnostics;
    JsonSpanScanner scanner(*source_);
    std::size_t featureIndex = 0;
    const bool scanned = scanner.ScanArrayElements(
        featureArraySpan_, [&](const SourceSpan& featureSpan) {
            try {
                std::vector<Diagnostic> diagnostics;
                Feature feature = ParseFeature(
                    ParseSpan(*source_, featureSpan), featureIndex, options_,
                    diagnostics);
                IncludeBounds(computedBounds, ComputeBounds(feature.geometry));
                featureDiagnostics.insert(featureDiagnostics.end(),
                                          diagnostics.begin(), diagnostics.end());
                ++featureIndex;
                return true;
            } catch (const std::exception&) {
                return false;
            }
        });
    if (!scanned) {
        featureDiagnostics.push_back(Issue(
            DiagnosticCode::MalformedJson, Severity::Error,
            "malformed JSON"));
    }

    metadata_.computedBounds = computedBounds;
    if (metadata_.declaredBounds.has_value() &&
        !BoundsEqual(*metadata_.declaredBounds, computedBounds)) {
        featureDiagnostics.push_back(Issue(
            DiagnosticCode::BoundsMismatch,
            options_.strict ? Severity::Error : Severity::Warning,
            "declared bounds disagree with computed bounds"));
    }
    diagnostics_.insert(diagnostics_.end(), featureDiagnostics.begin(),
                       featureDiagnostics.end());
    if (!scanned || HasErrors(featureDiagnostics)) {
        lazyMetadataFailed_ = true;
        return false;
    }
    lazyMetadataComplete_ = true;
    return true;
}

Result<std::optional<Feature>> Reader::ReadNext() {
    if (lazyExhausted_) {
        return Result<std::optional<Feature>>::Success(std::nullopt);
    }
    if (source_) {
        if (lazyReadFailed_) {
            return Result<std::optional<Feature>>::Failure(
                lazyReadFailureDiagnostics_);
        }
        if (lazyMetadataFailed_) {
            return Result<std::optional<Feature>>::Failure(diagnostics_);
        }
        if (nextFeature_ == featureCount_) {
            if (!lazyMetadataComplete_) {
                metadata_.computedBounds = lazyComputedBounds_;
                diagnostics_.insert(diagnostics_.end(),
                                   lazyFeatureDiagnostics_.begin(),
                                   lazyFeatureDiagnostics_.end());
                if (metadata_.declaredBounds.has_value() &&
                    !BoundsEqual(*metadata_.declaredBounds,
                                 *metadata_.computedBounds)) {
                    diagnostics_.push_back(Issue(
                        DiagnosticCode::BoundsMismatch,
                        options_.strict ? Severity::Error : Severity::Warning,
                        "declared bounds disagree with computed bounds"));
                }
                lazyMetadataComplete_ = true;
                if (HasErrors(diagnostics_)) {
                    lazyMetadataFailed_ = true;
                    return Result<std::optional<Feature>>::Failure(diagnostics_);
                }
            }
            source_.reset();
            featureArraySpan_ = {};
            nextFeaturePosition_ = 0;
            lazyExhausted_ = true;
            return Result<std::optional<Feature>>::Success(std::nullopt);
        }
        std::vector<Diagnostic> diagnostics;
        JsonSpanScanner scanner(*source_);
        SourceSpan span;
        if (!scanner.ScanNextArrayElement(featureArraySpan_,
                                          nextFeaturePosition_, span)) {
            lazyReadFailureDiagnostics_ = {Issue(
                DiagnosticCode::MalformedJson, Severity::Error,
                "malformed feature array", nextFeature_)};
            lazyReadFailed_ = true;
            return Result<std::optional<Feature>>::Failure(
                lazyReadFailureDiagnostics_);
        }
        Feature feature = ParseFeature(
            ParseSpan(*source_, span), nextFeature_, options_, diagnostics);
        if (HasErrors(diagnostics)) {
            lazyReadFailureDiagnostics_ = std::move(diagnostics);
            lazyReadFailed_ = true;
            return Result<std::optional<Feature>>::Failure(
                lazyReadFailureDiagnostics_);
        }
        IncludeBounds(lazyComputedBounds_, ComputeBounds(feature.geometry));
        lazyFeatureDiagnostics_.insert(lazyFeatureDiagnostics_.end(),
                                       diagnostics.begin(), diagnostics.end());
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
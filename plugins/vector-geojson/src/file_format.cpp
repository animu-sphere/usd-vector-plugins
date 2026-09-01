#include "usdvector/plugin/file_format.h"

#include "usdvector/authoring/authoring.h"
#include "usdvector/authoring/usd_authoring.h"
#include "usdvector/geojson/reader.h"

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/sdf/layer.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct ReadOptions {
    bool strict = false;
    bool includeProperties = true;
    enum class GeometryMode { All, Points, Curves, Meshes, None } geometry =
        GeometryMode::All;
};

bool ParseBoolean(const std::string& value, bool& result) {
    if (value == "true") {
        result = true;
        return true;
    }
    if (value == "false") {
        result = false;
        return true;
    }
    return false;
}

bool ParseArguments(const SdfFileFormat::FileFormatArguments& arguments,
                    ReadOptions& options, std::string& error) {
    for (const auto& [name, value] : arguments) {
        if (name.empty() || value.empty()) {
            error = "file-format arguments must not be empty";
            return false;
        }
        if (name == "strict") {
            if (!ParseBoolean(value, options.strict)) {
                error = "strict must be true or false";
                return false;
            }
        } else if (name == "properties") {
            if (value == "all") {
                options.includeProperties = true;
            } else if (value == "none") {
                options.includeProperties = false;
            } else {
                error = "properties must be all or none";
                return false;
            }
        } else if (name == "geometry") {
            if (value == "all") {
                options.geometry = ReadOptions::GeometryMode::All;
            } else if (value == "points") {
                options.geometry = ReadOptions::GeometryMode::Points;
            } else if (value == "curves") {
                options.geometry = ReadOptions::GeometryMode::Curves;
            } else if (value == "meshes") {
                options.geometry = ReadOptions::GeometryMode::Meshes;
            } else if (value == "none") {
                options.geometry = ReadOptions::GeometryMode::None;
            } else {
                error = "geometry must be all, points, curves, meshes, or none";
                return false;
            }
        } else {
            error = "unknown file-format argument: " + name;
            return false;
        }
    }
    return true;
}

void Report(const std::vector<usdvector::Diagnostic>& diagnostics) {
    for (const usdvector::Diagnostic& diagnostic : diagnostics) {
        const std::string message =
            std::string(usdvector::DiagnosticCodeString(diagnostic.code)) +
            ": " + diagnostic.message;
        if (diagnostic.severity == usdvector::Severity::Warning) {
            TF_WARN("%s", message.c_str());
        } else {
            TF_RUNTIME_ERROR("%s", message.c_str());
        }
    }
}

bool IsSelected(const usdvector::Geometry& geometry,
                ReadOptions::GeometryMode mode) {
    if (mode == ReadOptions::GeometryMode::All) {
        return true;
    }
    if (mode == ReadOptions::GeometryMode::None) {
        return false;
    }
    return std::visit(
        [mode](const auto& value) {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, usdvector::Point> ||
                          std::is_same_v<Value, usdvector::MultiPoint>) {
                return mode == ReadOptions::GeometryMode::Points;
            } else if constexpr (
                std::is_same_v<Value, usdvector::LineString> ||
                std::is_same_v<Value, usdvector::MultiLineString>) {
                return mode == ReadOptions::GeometryMode::Curves;
            } else if constexpr (std::is_same_v<Value, usdvector::Polygon> ||
                                 std::is_same_v<Value, usdvector::MultiPolygon>) {
                return mode == ReadOptions::GeometryMode::Meshes;
            } else {
                return false;
            }
        },
        geometry);
}

bool ReadSource(SdfLayer* layer, const std::string& source,
                const SdfFileFormat::FileFormatArguments& arguments,
                bool metadataOnly) {
    ReadOptions options;
    std::string argumentError;
    if (!ParseArguments(arguments, options, argumentError)) {
        TF_RUNTIME_ERROR("VGJSON001: %s", argumentError.c_str());
        return false;
    }

    auto readerResult = usdvector::geojson::Reader::Create(
        source, usdvector::geojson::ParseOptions{options.strict});
    Report(readerResult.diagnostics);
    if (!readerResult.Succeeded()) {
        return false;
    }
    auto metadataResult = readerResult.value->ReadMetadata();
    Report(metadataResult.diagnostics);
    if (!metadataResult.Succeeded()) {
        return false;
    }

    std::vector<usdvector::Feature> features;
    if (!metadataOnly && options.geometry != ReadOptions::GeometryMode::None) {
        while (true) {
            auto featureResult = readerResult.value->ReadNext();
            Report(featureResult.diagnostics);
            if (!featureResult.Succeeded()) {
                return false;
            }
            if (!featureResult.value->has_value()) {
                break;
            }
            usdvector::Feature feature = std::move(**featureResult.value);
            if (IsSelected(feature.geometry, options.geometry)) {
                if (!options.includeProperties) {
                    feature.properties.clear();
                }
                features.push_back(std::move(feature));
            }
        }
    }

    const auto planResult = usdvector::authoring::BuildAuthoringPlan(
        *metadataResult.value, features,
        usdvector::authoring::AuthoringOptions{options.strict});
    Report(planResult.diagnostics);
    if (!planResult.Succeeded()) {
        return false;
    }
    const auto stageResult =
        usdvector::authoring::BuildUsdStage(*planResult.value);
    Report(stageResult.diagnostics);
    if (!stageResult.Succeeded()) {
        return false;
    }
    layer->TransferContent((*stageResult.value)->GetRootLayer());
    return true;
}

std::string ReadAsset(const ArAsset& asset) {
    const auto buffer = asset.GetBuffer();
    if (!buffer) {
        return {};
    }
    return {buffer.get(), asset.GetSize()};
}

}  // namespace

PXR_NAMESPACE_OPEN_SCOPE

UsdVectorGeoJsonFileFormat::UsdVectorGeoJsonFileFormat()
    : SdfFileFormat(TfToken("GeoJSON"), TfToken("1.0"), TfToken("usd"),
                    std::vector<std::string>{"geojson", "json"}) {}

bool UsdVectorGeoJsonFileFormat::CanRead(const std::string& filePath) const {
    std::string extension;
    const std::size_t dot = filePath.find_last_of('.');
    if (dot != std::string::npos) {
        extension = filePath.substr(dot + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char value) {
                           return static_cast<char>(std::tolower(value));
                       });
    }
    return extension == "geojson" || extension == "json";
}

bool UsdVectorGeoJsonFileFormat::Read(
    SdfLayer* layer, const std::string& resolvedPath, bool metadataOnly) const {
    const auto asset =
        ArGetResolver().OpenAsset(ArResolvedPath(resolvedPath));
    if (!asset) {
        TF_RUNTIME_ERROR("VGJSON002: could not open source asset");
        return false;
    }
    return Read(layer, resolvedPath, asset.get(), metadataOnly);
}

bool UsdVectorGeoJsonFileFormat::Read(
    SdfLayer* layer, const std::string&, ArAsset* asset, bool metadataOnly) const {
    if (asset == nullptr) {
        TF_RUNTIME_ERROR("VGJSON002: source asset was not provided");
        return false;
    }
    const std::string source = ReadAsset(*asset);
    if (source.empty()) {
        TF_RUNTIME_ERROR("VGJSON002: source asset is empty or unreadable");
        return false;
    }
    return ReadSource(layer, source, layer->GetFileFormatArguments(), metadataOnly);
}

bool UsdVectorGeoJsonFileFormat::ReadFromString(SdfLayer* layer,
                                                 const std::string& content) const {
    return ReadSource(layer, content, layer->GetFileFormatArguments(), false);
}

bool UsdVectorGeoJsonFileFormat::WriteToString(
    const SdfLayer& layer, std::string* result, const std::string& comment) const {
    const SdfFileFormatConstPtr usdaFormat =
        SdfFileFormat::FindByExtension("anonymous.usda");
    return usdaFormat && usdaFormat->WriteToString(layer, result, comment);
}

PXR_NAMESPACE_CLOSE_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    SDF_DEFINE_FILE_FORMAT(UsdVectorGeoJsonFileFormat, SdfFileFormat);
}
#include "usdvector/plugin/read_options.h"

#include <cctype>
#include <cmath>
#include <locale>
#include <sstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace usdvector::plugin {
namespace {

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

bool ParsePositiveFinite(const std::string& value, double& result) {
    if (value.empty() ||
        std::isspace(static_cast<unsigned char>(value.front()))) {
        return false;
    }
    std::istringstream stream(value);
    stream.imbue(std::locale::classic());
    double parsed = 0.0;
    stream >> parsed;
    if (stream.fail() || stream.get() != std::char_traits<char>::eof()) {
        return false;
    }
    if (!std::isfinite(parsed) || parsed <= 0.0) {
        return false;
    }
    result = parsed;
    return true;
}

}  // namespace

bool ParseReadOptions(const SdfFileFormat::FileFormatArguments& arguments,
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
        } else if (name == "upAxis") {
            if (value == "y") {
                options.stage.upAxis = authoring::StageUpAxis::Y;
            } else if (value == "z") {
                options.stage.upAxis = authoring::StageUpAxis::Z;
            } else {
                error = "upAxis must be y or z";
                return false;
            }
        } else if (name == "metersPerUnit") {
            double metersPerUnit = 0.0;
            if (!ParsePositiveFinite(value, metersPerUnit)) {
                error = "metersPerUnit must be a positive finite number";
                return false;
            }
            options.stage.metersPerUnit = metersPerUnit;
        } else {
            error = "unknown file-format argument: " + name;
            return false;
        }
    }
    return true;
}

}  // namespace usdvector::plugin

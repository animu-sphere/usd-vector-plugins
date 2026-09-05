#pragma once

#include "usdvector/authoring/usd_authoring.h"

#include <string>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/fileFormat.h>

namespace usdvector::plugin {

struct ReadOptions {
    enum class GeometryMode { All, Points, Curves, Meshes, None };

    bool strict = false;
    bool includeProperties = true;
    GeometryMode geometry = GeometryMode::All;
    authoring::StageOptions stage;
};

bool ParseReadOptions(const pxr::SdfFileFormat::FileFormatArguments& arguments,
                      ReadOptions& options, std::string& error);

}  // namespace usdvector::plugin

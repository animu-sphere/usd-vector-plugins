#pragma once

#include <pxr/pxr.h>
#include <pxr/usd/ar/asset.h>
#include <pxr/usd/sdf/fileFormat.h>

PXR_NAMESPACE_OPEN_SCOPE

class UsdVectorGeoJsonFileFormat final : public SdfFileFormat {
public:
    UsdVectorGeoJsonFileFormat();

    bool CanRead(const std::string& filePath) const override;
    bool Read(SdfLayer* layer, const std::string& resolvedPath,
              bool metadataOnly = false) const override;
    bool Read(SdfLayer* layer, const std::string& resolvedPath,
              ArAsset* asset, bool metadataOnly = false) const;
    bool ReadFromString(SdfLayer* layer, const std::string& content) const override;
    bool WriteToString(const SdfLayer& layer, std::string* result,
                       const std::string& comment = std::string()) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE
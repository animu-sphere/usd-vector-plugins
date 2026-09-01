#pragma once

#include "usdvector/core/diagnostics.h"
#include "usdvector/core/model.h"

#include <optional>

namespace usdvector {

class FeatureReader {
public:
    virtual Result<DatasetMetadata> ReadMetadata() = 0;
    virtual Result<std::optional<Feature>> ReadNext() = 0;
    virtual ~FeatureReader() = default;
};

}  // namespace usdvector
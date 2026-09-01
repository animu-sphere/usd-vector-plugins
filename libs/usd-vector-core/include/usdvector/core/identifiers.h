#pragma once

#include "usdvector/core/model.h"

#include <cstddef>
#include <string>

namespace usdvector {

std::string NormalizeIdentifier(const std::string& source);
std::string MakeFeatureName(const FeatureId& id);
std::string MakeFeatureName(std::size_t sourceIndex);

}  // namespace usdvector
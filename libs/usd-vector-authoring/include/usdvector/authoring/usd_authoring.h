#pragma once

#include "usdvector/authoring/authoring.h"

#include <optional>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

namespace usdvector::authoring {

enum class StageUpAxis {
    Y,
    Z,
};

struct StageOptions {
    std::optional<StageUpAxis> upAxis;
    std::optional<double> metersPerUnit;
};

Result<pxr::UsdStageRefPtr> BuildUsdStage(const AuthoringPlan& plan,
                                          const StageOptions& options = {});

}  // namespace usdvector::authoring

#pragma once

#include "usdvector/authoring/authoring.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

namespace usdvector::authoring {

Result<pxr::UsdStageRefPtr> BuildUsdStage(const AuthoringPlan& plan);

}  // namespace usdvector::authoring
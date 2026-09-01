#pragma once

#include "usdvector/core/diagnostics.h"
#include "usdvector/core/geometry.h"

#include <vector>

namespace usdvector {

struct ValidationOptions {
    bool strict = false;
};

std::vector<Diagnostic> ValidateGeometry(
    const Geometry& geometry,
    const ValidationOptions& options = {});

Result<Ring> NormalizeRing(const Ring& ring);

}  // namespace usdvector
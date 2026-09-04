#pragma once

#include "usdvector/core/diagnostics.h"
#include "usdvector/core/model.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace usdvector {

class FeatureReader {
public:
    virtual Result<DatasetMetadata> ReadMetadata() = 0;
    virtual Result<std::optional<Feature>> ReadNext() = 0;

    Result<std::vector<Feature>> ReadBatch(std::size_t maxFeatures) {
        std::vector<Feature> batch;
        std::vector<Diagnostic> diagnostics;
        while (batch.size() < maxFeatures) {
            Result<std::optional<Feature>> next = ReadNext();
            diagnostics.insert(diagnostics.end(), next.diagnostics.begin(),
                               next.diagnostics.end());
            if (!next.Succeeded()) {
                return Result<std::vector<Feature>>::Failure(
                    std::move(diagnostics));
            }
            if (!next.value->has_value()) {
                break;
            }
            batch.push_back(std::move(next.value->value()));
        }
        Result<std::vector<Feature>> result =
            Result<std::vector<Feature>>::Success(std::move(batch));
        result.diagnostics = std::move(diagnostics);
        return result;
    }

    virtual ~FeatureReader() = default;
};

}  // namespace usdvector
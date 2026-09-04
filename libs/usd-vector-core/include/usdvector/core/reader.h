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
        while (batch.size() < maxFeatures) {
            Result<std::optional<Feature>> next = ReadNext();
            if (!next.Succeeded()) {
                return Result<std::vector<Feature>>::Failure(
                    std::move(next.diagnostics));
            }
            if (!next.value->has_value()) {
                break;
            }
            batch.push_back(std::move(next.value->value()));
        }
        return Result<std::vector<Feature>>::Success(std::move(batch));
    }

    virtual ~FeatureReader() = default;
};

}  // namespace usdvector
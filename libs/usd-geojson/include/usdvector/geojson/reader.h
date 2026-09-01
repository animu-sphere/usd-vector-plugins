#pragma once

#include "usdvector/core/reader.h"

#include <string>
#include <string_view>
#include <vector>

namespace usdvector::geojson {

struct ParseOptions {
    bool strict = false;
};

Result<DatasetMetadata> ParseMetadata(std::string_view source,
                                      const ParseOptions& options = {});

class Reader final : public FeatureReader {
public:
    static Result<Reader> Create(std::string source,
                                  const ParseOptions& options = {});

    Result<DatasetMetadata> ReadMetadata() override;
    Result<std::optional<Feature>> ReadNext() override;

private:
    Reader(DatasetMetadata metadata, std::vector<Feature> features,
           std::vector<Diagnostic> diagnostics);

    DatasetMetadata metadata_;
    std::vector<Feature> features_;
    std::vector<Diagnostic> diagnostics_;
    std::size_t nextFeature_ = 0;
};

}  // namespace usdvector::geojson
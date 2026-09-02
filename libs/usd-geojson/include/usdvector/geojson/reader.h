#pragma once

#include "usdvector/core/reader.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
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
    static Result<Reader> CreateLazy(std::string source,
                                      const ParseOptions& options = {});

    Result<DatasetMetadata> ReadMetadata() override;
    Result<std::optional<Feature>> ReadNext() override;

private:
    Reader(DatasetMetadata metadata, std::vector<Feature> features,
           std::vector<Diagnostic> diagnostics,
           std::shared_ptr<const std::string> source = {},
           std::vector<std::pair<std::size_t, std::size_t>> featureSpans = {},
           ParseOptions options = {});

    DatasetMetadata metadata_;
    std::vector<Feature> features_;
    std::vector<Diagnostic> diagnostics_;
    std::shared_ptr<const std::string> source_;
    std::vector<std::pair<std::size_t, std::size_t>> featureSpans_;
    ParseOptions options_;
    std::size_t nextFeature_ = 0;
};

}  // namespace usdvector::geojson
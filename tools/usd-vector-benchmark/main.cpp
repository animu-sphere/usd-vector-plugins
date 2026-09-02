#include "usdvector/authoring/authoring.h"
#include "usdvector/geojson/reader.h"

#if defined(USDVECTOR_ENABLE_OPENUSD)
#include "usdvector/authoring/usd_authoring.h"
#endif

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#if defined(_WIN32)
#define PSAPI_VERSION 1
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;

struct BenchmarkCase {
    std::string name;
    std::size_t count;
};

struct Metrics {
    std::string reader;
    std::string name;
    std::size_t requestedCount = 0;
    std::size_t sourceBytes = 0;
    std::size_t featureCount = 0;
    std::size_t vertexCount = 0;
    double parseMilliseconds = 0.0;
    double firstFeatureMilliseconds = 0.0;
    double authoringMilliseconds = 0.0;
    double timeToOpenMilliseconds = 0.0;
    std::uint64_t peakRssBytes = 0;
    std::size_t copiedBytes = 0;
    std::size_t retainedFeatureBytes = 0;
#if defined(USDVECTOR_ENABLE_OPENUSD)
    std::optional<double> usdEmissionMilliseconds;
    std::optional<std::size_t> flattenedLayerBytes;
#endif
};

void AppendNumber(std::string& output, double value) {
    output += std::to_string(value);
}

void AppendFeaturePrefix(std::string& output, std::size_t index) {
    output += R"({"type":"Feature","id":"f)";
    output += std::to_string(index);
    output += R"(","geometry":)";
}

void AppendPoint(std::string& output, double x, double y) {
    output += '[';
    AppendNumber(output, x);
    output += ',';
    AppendNumber(output, y);
    output += ']';
}

void AppendProperties(std::string& output, std::size_t index,
                      bool propertyHeavy) {
    if (!propertyHeavy) {
        output += "{}";
        return;
    }
    output += '{';
    for (std::size_t property = 0; property < 32; ++property) {
        if (property != 0) {
            output += ',';
        }
        output += "\"value";
        output += std::to_string(property);
        output += "\":";
        if (property % 4 == 0) {
            output += '"';
            output += "feature-";
            output += std::to_string(index);
            output += '"';
        } else if (property % 4 == 1) {
            output += std::to_string(index * (property + 1));
        } else if (property % 4 == 2) {
            AppendNumber(output, static_cast<double>(index) * 0.25);
        } else {
            output += (index % 2 == 0) ? "true" : "false";
        }
    }
    output += '}';
}

std::string BuildSource(const BenchmarkCase& benchmarkCase) {
    const bool knownCase = benchmarkCase.name == "points" ||
                           benchmarkCase.name == "lines" ||
                           benchmarkCase.name == "large-polygon" ||
                           benchmarkCase.name == "small-polygons" ||
                           benchmarkCase.name == "property-heavy" ||
                           benchmarkCase.name == "large-coordinates";
    if (!knownCase) {
        throw std::invalid_argument("unknown benchmark case: " + benchmarkCase.name);
    }
    std::string output;
    output.reserve(benchmarkCase.count * 96);
    output += R"({"type":"FeatureCollection","features":[)";

    const auto appendFeatureSeparator = [&output](std::size_t index) {
        if (index != 0) {
            output += ',';
        }
    };

    if (benchmarkCase.name == "large-polygon") {
        std::ostringstream polygon;
        polygon << R"({"type":"FeatureCollection","features":[{"type":"Feature","id":"polygon","geometry":{"type":"Polygon","coordinates":[[)";
        const std::size_t vertexCount = std::max<std::size_t>(
            benchmarkCase.count, 3);
        for (std::size_t vertex = 0; vertex <= vertexCount; ++vertex) {
            if (vertex != 0) {
                polygon << ',';
            }
            const auto point = [&](double x, double y) {
                polygon << '[' << std::setprecision(17) << x << ',' << y << ']';
            };
            if (vertex == vertexCount) {
                point(1.0, 0.0);
            } else {
                const double angle =
                    6.28318530717958647692 * static_cast<double>(vertex) /
                    static_cast<double>(vertexCount);
                point(std::cos(angle), std::sin(angle));
            }
        }
        polygon << R"(]]},"properties":{}}]})";
        return polygon.str();
    } else {
        for (std::size_t index = 0; index < benchmarkCase.count; ++index) {
            appendFeatureSeparator(index);
            AppendFeaturePrefix(output, index);
            if (benchmarkCase.name == "lines") {
                output += R"({"type":"LineString","coordinates":[)";
                for (std::size_t vertex = 0; vertex < 16; ++vertex) {
                    if (vertex != 0) {
                        output += ',';
                    }
                    AppendPoint(output, static_cast<double>(index) + vertex * 0.1,
                                static_cast<double>(vertex));
                }
                output += ']';
                output += '}';
            } else if (benchmarkCase.name == "small-polygons") {
                output += R"({"type":"Polygon","coordinates":[[)";
                AppendPoint(output, static_cast<double>(index), 0.0);
                output += ',';
                AppendPoint(output, static_cast<double>(index) + 1.0, 0.0);
                output += ',';
                AppendPoint(output, static_cast<double>(index) + 1.0, 1.0);
                output += ',';
                AppendPoint(output, static_cast<double>(index), 1.0);
                output += ',';
                AppendPoint(output, static_cast<double>(index), 0.0);
                output += "]]";
                output += '}';
            } else {
                output += R"({"type":"Point","coordinates":)";
                const double offset = benchmarkCase.name == "large-coordinates"
                                          ? 1000000000000.0
                                          : 0.0;
                AppendPoint(output, offset + static_cast<double>(index) * 0.25,
                            offset + static_cast<double>(index) * 0.5);
                output += '}';
            }
            output += ",\"properties\":";
            AppendProperties(output, index,
                             benchmarkCase.name == "property-heavy");
            output += '}';
        }
    }

    output += "]}";
    return output;
}

std::size_t GeometryVertexCount(const usdvector::Geometry& geometry) {
    return std::visit(
        [](const auto& value) -> std::size_t {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::monostate>) {
                return 0;
            } else if constexpr (std::is_same_v<Value, usdvector::Point>) {
                return 1;
            } else if constexpr (std::is_same_v<Value, usdvector::MultiPoint> ||
                                 std::is_same_v<Value, usdvector::LineString>) {
                return value.coordinates.size();
            } else if constexpr (std::is_same_v<Value, usdvector::MultiLineString>) {
                std::size_t count = 0;
                for (const auto& line : value.lines) {
                    count += line.coordinates.size();
                }
                return count;
            } else if constexpr (std::is_same_v<Value, usdvector::Polygon>) {
                std::size_t count = value.outer.size();
                for (const auto& hole : value.holes) {
                    count += hole.size();
                }
                return count;
            } else {
                std::size_t count = 0;
                for (const auto& polygon : value.polygons) {
                    count += polygon.outer.size();
                    for (const auto& hole : polygon.holes) {
                        count += hole.size();
                    }
                }
                return count;
            }
        },
        geometry);
}

std::size_t GeometryBytes(const usdvector::Geometry& geometry) {
    return std::visit(
        [](const auto& value) -> std::size_t {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::monostate> ||
                          std::is_same_v<Value, usdvector::Point>) {
                return 0;
            } else if constexpr (std::is_same_v<Value, usdvector::MultiPoint> ||
                                 std::is_same_v<Value, usdvector::LineString>) {
                return value.coordinates.capacity() * sizeof(usdvector::Coordinate);
            } else if constexpr (std::is_same_v<Value, usdvector::MultiLineString>) {
                std::size_t bytes =
                    value.lines.capacity() * sizeof(usdvector::LineString);
                for (const auto& line : value.lines) {
                    bytes += line.coordinates.capacity() * sizeof(usdvector::Coordinate);
                }
                return bytes;
            } else if constexpr (std::is_same_v<Value, usdvector::Polygon>) {
                std::size_t bytes =
                                    value.outer.capacity() * sizeof(usdvector::Coordinate) +
                                    value.holes.capacity() * sizeof(usdvector::Ring);
                for (const auto& hole : value.holes) {
                    bytes += hole.capacity() * sizeof(usdvector::Coordinate);
                }
                return bytes;
            } else {
                std::size_t bytes =
                                    value.polygons.capacity() * sizeof(usdvector::Polygon);
                for (const auto& polygon : value.polygons) {
                    bytes += polygon.outer.capacity() * sizeof(usdvector::Coordinate);
                    bytes += polygon.holes.capacity() * sizeof(usdvector::Ring);
                    for (const auto& hole : polygon.holes) {
                        bytes += hole.capacity() * sizeof(usdvector::Coordinate);
                    }
                }
                return bytes;
            }
        },
        geometry);
}

std::size_t PropertyBytes(const usdvector::PropertyValue& property) {
    return std::visit(
        [](const auto& value) -> std::size_t {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::monostate> ||
                          std::is_same_v<Value, bool> ||
                          std::is_same_v<Value, std::int64_t> ||
                          std::is_same_v<Value, std::uint64_t> ||
                          std::is_same_v<Value, double>) {
                return 0;
            } else if constexpr (std::is_same_v<Value, std::string>) {
                return value.capacity();
            } else if constexpr (std::is_same_v<Value, usdvector::PropertyValue::Array>) {
                std::size_t bytes =
                    value.capacity() * sizeof(usdvector::PropertyValue);
                for (const auto& item : value) {
                    bytes += PropertyBytes(item);
                }
                return bytes;
            } else {
                std::size_t bytes =
                    value.size() * sizeof(std::pair<const std::string,
                                                     usdvector::PropertyValue>);
                for (const auto& [name, item] : value) {
                    bytes += name.capacity() + PropertyBytes(item);
                }
                return bytes;
            }
        },
        property.value);
}

std::size_t FeatureBytes(const usdvector::Feature& feature) {
    std::size_t bytes = sizeof(feature) + GeometryBytes(feature.geometry) +
                        feature.properties.size() *
                            sizeof(std::pair<const std::string,
                                             usdvector::PropertyValue>);
    for (const auto& [name, property] : feature.properties) {
        bytes += name.capacity() + PropertyBytes(property);
    }
    return bytes;
}

std::uint64_t PeakRssBytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
    }
    return 0;
#else
    struct rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }
#if defined(__APPLE__)
    return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
#endif
#endif
}

std::string DiagnosticSummary(const std::vector<usdvector::Diagnostic>& diagnostics) {
    if (diagnostics.empty()) {
        return {};
    }
    return usdvector::DiagnosticCodeString(diagnostics.front().code);
}

Metrics Measure(const BenchmarkCase& benchmarkCase, bool lazy) {
    Metrics metrics;
    metrics.reader = lazy ? "lazy" : "buffered";
    metrics.name = benchmarkCase.name;
    metrics.requestedCount = benchmarkCase.count;
    std::string source = BuildSource(benchmarkCase);
    metrics.sourceBytes = source.size();
    metrics.copiedBytes = 0;

    const auto openStart = Clock::now();
    auto reader = lazy
                      ? usdvector::geojson::Reader::CreateLazy(std::move(source))
                      : usdvector::geojson::Reader::Create(std::move(source));
    const auto opened = Clock::now();
    if (!reader.Succeeded()) {
        throw std::runtime_error("benchmark source could not be parsed: " +
                                 DiagnosticSummary(reader.diagnostics));
    }
    metrics.parseMilliseconds =
        std::chrono::duration<double, std::milli>(opened - openStart).count();
    metrics.timeToOpenMilliseconds = metrics.parseMilliseconds;

    auto metadata = reader.value->ReadMetadata();
    auto first = reader.value->ReadNext();
    const auto firstFeature = Clock::now();
    if (!metadata.Succeeded() || !first.Succeeded() || !first.value.has_value() ||
        !first.value->has_value()) {
        throw std::runtime_error("benchmark reader did not return its first feature");
    }
    metrics.firstFeatureMilliseconds =
        std::chrono::duration<double, std::milli>(firstFeature - openStart).count();

    const std::size_t expectedFeatures = metadata.value->featureCount.value_or(0);
    std::vector<usdvector::Feature> features;
    features.reserve(expectedFeatures);
    features.push_back(std::move(first.value->value()));
    while (true) {
        auto next = reader.value->ReadNext();
        if (!next.Succeeded()) {
            throw std::runtime_error("benchmark reader failed during iteration");
        }
        if (!next.value.has_value() || !next.value->has_value()) {
            break;
        }
        features.push_back(std::move(next.value->value()));
    }
    metrics.featureCount = features.size();
    for (const auto& feature : features) {
        metrics.vertexCount += GeometryVertexCount(feature.geometry);
        metrics.retainedFeatureBytes += FeatureBytes(feature);
    }

    const auto authoringStart = Clock::now();
    auto plan = usdvector::authoring::BuildAuthoringPlan(
        *metadata.value, features);
    const auto authored = Clock::now();
    if (!plan.Succeeded()) {
        throw std::runtime_error("benchmark authoring plan could not be built: " +
                                 DiagnosticSummary(plan.diagnostics));
    }
    metrics.authoringMilliseconds =
        std::chrono::duration<double, std::milli>(authored - authoringStart).count();

#if defined(USDVECTOR_ENABLE_OPENUSD)
    const auto emissionStart = Clock::now();
    auto stage = usdvector::authoring::BuildUsdStage(*plan.value);
    const auto emitted = Clock::now();
    if (!stage.Succeeded()) {
        throw std::runtime_error("benchmark USD stage could not be built");
    }
    std::string flattened;
    if (!stage.value->GetRootLayer()->ExportToString(&flattened)) {
        throw std::runtime_error("benchmark USD layer could not be flattened");
    }
    metrics.usdEmissionMilliseconds =
        std::chrono::duration<double, std::milli>(emitted - emissionStart).count();
    metrics.flattenedLayerBytes = flattened.size();
#endif

    metrics.peakRssBytes = PeakRssBytes();
    return metrics;
}

bool WriteHeader(std::ostream& output) {
    output << "reader,case,requested_count,source_bytes,features,vertices,parse_ms,"
              "time_to_first_feature_ms,authoring_plan_ms,time_to_open_ms,"
              "peak_rss_bytes,copied_bytes,retained_feature_bytes";
#if defined(USDVECTOR_ENABLE_OPENUSD)
    output << ",usd_emission_ms,flattened_layer_bytes";
#endif
    output << '\n';
    return static_cast<bool>(output);
}

bool WriteMetrics(std::ostream& output, const Metrics& metrics) {
    output << metrics.reader << ',' << metrics.name << ','
           << metrics.requestedCount << ','
           << metrics.sourceBytes << ',' << metrics.featureCount << ','
           << metrics.vertexCount << ',' << std::setprecision(12)
           << metrics.parseMilliseconds << ',' << metrics.firstFeatureMilliseconds
           << ',' << metrics.authoringMilliseconds << ','
           << metrics.timeToOpenMilliseconds << ',' << metrics.peakRssBytes << ','
           << metrics.copiedBytes << ',' << metrics.retainedFeatureBytes;
#if defined(USDVECTOR_ENABLE_OPENUSD)
    output << ',';
    if (metrics.usdEmissionMilliseconds.has_value()) {
        output << *metrics.usdEmissionMilliseconds;
    }
    output << ',';
    if (metrics.flattenedLayerBytes.has_value()) {
        output << *metrics.flattenedLayerBytes;
    }
#endif
    output << '\n';
    return static_cast<bool>(output);
}

bool ParseCount(std::string_view text, std::size_t& count) {
    const auto result = std::from_chars(text.data(), text.data() + text.size(), count);
    return result.ec == std::errc() && result.ptr == text.data() + text.size() &&
           count > 0;
}

std::vector<BenchmarkCase> Cases(const std::optional<std::string>& selected,
                                 std::size_t count) {
    if (selected.has_value()) {
        return {{*selected, count}};
    }
    return {{"points", 1000},
            {"points", 100000},
            {"lines", 1000},
            {"large-polygon", 1000},
            {"small-polygons", 1000},
            {"property-heavy", 1000},
            {"large-coordinates", 1000}};
}

void PrintUsage() {
    std::cerr << "usage: usd-vector-benchmark [--reader MODE] [--case NAME] [--count N] [--output FILE]\n"
                 "readers: buffered, lazy\n"
                 "cases: points, lines, large-polygon, small-polygons, "
                 "property-heavy, large-coordinates\n";
}

}  // namespace

int main(int argc, char** argv) {
    bool lazy = false;
    std::optional<std::string> selectedCase;
    std::size_t count = 1000;
    std::optional<std::string> outputPath;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--reader" && index + 1 < argc) {
            const std::string reader = argv[++index];
            if (reader != "buffered" && reader != "lazy") {
                std::cerr << "--reader must be buffered or lazy\n";
                return 2;
            }
            lazy = reader == "lazy";
        } else if (argument == "--case" && index + 1 < argc) {
            selectedCase = argv[++index];
        } else if (argument == "--count" && index + 1 < argc) {
            if (!ParseCount(argv[++index], count)) {
                std::cerr << "--count must be a positive integer\n";
                return 2;
            }
        } else if (argument == "--output" && index + 1 < argc) {
            outputPath = argv[++index];
        } else if (argument == "--help") {
            PrintUsage();
            return 0;
        } else {
            PrintUsage();
            return 2;
        }
    }
    if (count == 0) {
        std::cerr << "--count must be greater than zero\n";
        return 2;
    }

    std::ofstream file;
    std::ostream* output = &std::cout;
    if (outputPath.has_value()) {
        file.open(*outputPath, std::ios::binary);
        if (!file) {
            std::cerr << "could not open output file\n";
            return 2;
        }
        output = &file;
    }

    try {
        if (!WriteHeader(*output)) {
            std::cerr << "could not write benchmark output\n";
            return 1;
        }
        for (const auto& benchmarkCase : Cases(selectedCase, count)) {
            const Metrics metrics = Measure(benchmarkCase, lazy);
            if (!WriteMetrics(*output, metrics)) {
                std::cerr << "could not write benchmark output\n";
                return 1;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}

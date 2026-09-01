#include "usdvector/core/identifiers.h"

#include <charconv>
#include <cstdint>
#include <type_traits>

namespace usdvector {
namespace {

bool IsAsciiLetter(unsigned char value) {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}

bool IsAsciiDigit(unsigned char value) {
    return value >= '0' && value <= '9';
}

std::string IntegerText(std::int64_t value) {
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

std::string IntegerText(std::uint64_t value) {
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

}  // namespace

std::string NormalizeIdentifier(const std::string& source) {
    std::string normalized;
    normalized.reserve(source.size() + 1);
    for (unsigned char value : source) {
        normalized.push_back(IsAsciiLetter(value) || IsAsciiDigit(value) ||
                                     value == '_'
                                 ? static_cast<char>(value)
                                 : '_');
    }
    if (normalized.empty()) {
        return "_";
    }
    if (IsAsciiDigit(static_cast<unsigned char>(normalized.front()))) {
        normalized.insert(normalized.begin(), '_');
    }
    return normalized;
}

std::string MakeFeatureName(const FeatureId& id) {
    const std::string source = std::visit(
        [](const auto& value) -> std::string {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, std::string>) {
                return value;
            } else {
                return IntegerText(value);
            }
        },
        id);
    return "id_" + NormalizeIdentifier(source);
}

std::string MakeFeatureName(std::size_t sourceIndex) {
    std::string sequence = std::to_string(sourceIndex);
    if (sequence.size() < 8) {
        sequence.insert(sequence.begin(), 8 - sequence.size(), '0');
    }
    return "f_" + sequence;
}

}  // namespace usdvector
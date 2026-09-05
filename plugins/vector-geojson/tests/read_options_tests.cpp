#include "usdvector/plugin/read_options.h"

#include <cassert>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

using usdvector::plugin::ParseReadOptions;
using usdvector::plugin::ReadOptions;

bool Accepts(const SdfFileFormat::FileFormatArguments& arguments,
             ReadOptions& options) {
    std::string error;
    const bool parsed = ParseReadOptions(arguments, options, error);
    assert(parsed == error.empty());
    return parsed;
}

bool Rejects(const SdfFileFormat::FileFormatArguments& arguments) {
    ReadOptions options;
    return !Accepts(arguments, options);
}

void TestStagePolicyDefaults() {
    ReadOptions options;
    assert(Accepts({}, options));
    assert(!options.stage.upAxis.has_value());
    assert(!options.stage.metersPerUnit.has_value());

    assert(Accepts({{"geometry", "points"}, {"strict", "true"}}, options));
    assert(!options.stage.upAxis.has_value());
    assert(!options.stage.metersPerUnit.has_value());
}

void TestStagePolicyOverrides() {
    ReadOptions yUp;
    assert(Accepts({{"upAxis", "y"}}, yUp));
    assert(yUp.stage.upAxis == usdvector::authoring::StageUpAxis::Y);

    ReadOptions zUp;
    assert(Accepts({{"upAxis", "z"}, {"metersPerUnit", "0.001"}}, zUp));
    assert(zUp.stage.upAxis == usdvector::authoring::StageUpAxis::Z);
    assert(zUp.stage.metersPerUnit.has_value());
    assert(*zUp.stage.metersPerUnit == 0.001);

    ReadOptions exponent;
    assert(Accepts({{"metersPerUnit", "1e-3"}}, exponent));
    assert(*exponent.stage.metersPerUnit == 0.001);

    ReadOptions fraction;
    assert(Accepts({{"metersPerUnit", ".5"}}, fraction));
    assert(*fraction.stage.metersPerUnit == 0.5);
}

void TestStagePolicyRejection() {
    assert(Rejects({{"upAxis", "x"}}));
    assert(Rejects({{"upAxis", "Y"}}));
    assert(Rejects({{"upAxis", ""}}));
    assert(Rejects({{"metersPerUnit", "0"}}));
    assert(Rejects({{"metersPerUnit", "-1"}}));
    assert(Rejects({{"metersPerUnit", "inf"}}));
    assert(Rejects({{"metersPerUnit", "nan"}}));
    assert(Rejects({{"metersPerUnit", "1e400"}}));
    assert(Rejects({{"metersPerUnit", "1 "}}));
    assert(Rejects({{"metersPerUnit", " 1"}}));
    assert(Rejects({{"metersPerUnit", "1meter"}}));
    assert(Rejects({{"metersPerUnit", "0x10"}}));
    assert(Rejects({{"metersPerUnit", "0X1P3"}}));
    assert(Rejects({{"metersPerUnit", "+1"}}));
    assert(Rejects({{"metersPerUnit", "1e"}}));
    assert(Rejects({{"metersPerUnit", "1e+"}}));
    assert(Rejects({{"metersPerUnit", "."}}));
    assert(Rejects({{"metersPerUnit", ""}}));
    assert(Rejects({{"upAxis", "z"}, {"unknown", "value"}}));
}

void TestExistingArguments() {
    ReadOptions options;
    assert(Accepts({{"strict", "true"},
                    {"properties", "none"},
                    {"geometry", "meshes"}},
                   options));
    assert(options.strict);
    assert(!options.includeProperties);
    assert(options.geometry == ReadOptions::GeometryMode::Meshes);
    assert(Rejects({{"strict", "yes"}}));
    assert(Rejects({{"properties", "some"}}));
    assert(Rejects({{"geometry", "solids"}}));
}

}  // namespace

int main() {
    TestStagePolicyDefaults();
    TestStagePolicyOverrides();
    TestStagePolicyRejection();
    TestExistingArguments();
    return 0;
}

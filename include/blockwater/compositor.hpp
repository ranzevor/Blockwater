#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace blockwater {

// Premultiplied RGBA in linear colour space; depths are positive metres.
struct Pixel { float r = 0, g = 0, b = 0, a = 0; };

struct CompositeInputs {
    std::span<const Pixel> host;
    std::span<const float> host_depth;
    std::span<const Pixel> guest_world;
    std::span<const float> guest_depth;
    std::span<const Pixel> guest_overlay;
    float bias_metres = 0.04f;
};

std::vector<Pixel> composite(const CompositeInputs& inputs);

} // namespace blockwater

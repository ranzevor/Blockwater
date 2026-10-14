#include "blockwater/compositor.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blockwater {

std::vector<Pixel> composite(const CompositeInputs& in) {
    const auto n = in.host.size();
    if (in.host_depth.size() != n || in.guest_world.size() != n ||
        in.guest_depth.size() != n || in.guest_overlay.size() != n ||
        !std::isfinite(in.bias_metres) || in.bias_metres < 0 || in.bias_metres > 1)
        throw std::invalid_argument("compositing inputs are inconsistent");
    std::vector<Pixel> output;
    output.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const bool guest_front = std::isfinite(in.guest_depth[i]) && in.guest_depth[i] >= 0 &&
            (in.guest_depth[i] <= in.host_depth[i] + in.bias_metres);
        Pixel p = in.host[i];
        if (guest_front) {
            const Pixel guest = in.guest_world[i];
            const float a = std::clamp(guest.a, 0.f, 1.f);
            p = {guest.r + p.r * (1-a), guest.g + p.g * (1-a),
                 guest.b + p.b * (1-a), a + p.a * (1-a)};
        }
        const Pixel hud = in.guest_overlay[i];
        const float hud_a = std::clamp(hud.a, 0.f, 1.f);
        p = {hud.r + p.r * (1-hud_a), hud.g + p.g * (1-hud_a),
             hud.b + p.b * (1-hud_a), hud_a + p.a * (1-hud_a)};
        output.push_back(p);
    }
    return output;
}

} // namespace blockwater

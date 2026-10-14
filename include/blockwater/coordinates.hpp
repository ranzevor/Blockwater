#pragma once
#include "blockwater/wire.hpp"

namespace blockwater {

// RDR2 is Z-up in metres; Minecraft is Y-up in blocks. Both origins are
// explicit to keep precision stable as the host player travels long distances.
struct WorldTransform {
    Vec3 host_origin;
    Vec3 guest_origin;
    double metres_per_block = 1;

    Vec3 host_to_guest(Vec3 h) const {
        const auto scale = 1 / metres_per_block;
        return {guest_origin.x + (h.x-host_origin.x)*scale,
                guest_origin.y + (h.z-host_origin.z)*scale,
                guest_origin.z - (h.y-host_origin.y)*scale};
    }
    Vec3 guest_to_host(Vec3 g) const {
        return {host_origin.x + (g.x-guest_origin.x)*metres_per_block,
                host_origin.y - (g.z-guest_origin.z)*metres_per_block,
                host_origin.z + (g.y-guest_origin.y)*metres_per_block};
    }
    bool valid() const {
        return finite(host_origin) && finite(guest_origin) &&
            std::isfinite(metres_per_block) && metres_per_block > 0;
    }
};

} // namespace blockwater

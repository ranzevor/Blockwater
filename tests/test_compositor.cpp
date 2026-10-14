#include "blockwater/compositor.hpp"
#include "test.hpp"
#include <cmath>
#include <vector>
#include <stdexcept>

using namespace blockwater;

void test_compositor() {
    const std::vector<Pixel> host(3, {1,0,0,1});
    const std::vector<float> host_z{5,5,5};
    const std::vector<Pixel> mc(3, {0,1,0,1});
    const std::vector<float> mc_z{3,7,8};
    const std::vector<Pixel> hud{{0,0,0,0},{0,0,0,0},{0,0,1,1}};
    const auto picture = composite({host,host_z,mc,mc_z,hud});
    REQUIRE(picture.size() == 3);
    REQUIRE(picture[0].g == 1 && picture[0].r == 0);
    REQUIRE(picture[1].r == 1 && picture[1].g == 0);
    REQUIRE(picture[2].b == 1 && picture[2].r == 0);
    const std::vector<Pixel> half(1,{0,0.5f,0,0.5f});
    const std::vector<Pixel> red(1,{1,0,0,1});
    const std::vector<float> near(1,1), far(1,8);
    const std::vector<Pixel> none(1,{0,0,0,0});
    const auto blended = composite({red,far,half,near,none});
    REQUIRE(std::abs(blended[0].r - 0.5) < 1e-6);
    REQUIRE(std::abs(blended[0].g - 0.5) < 1e-6);
    bool rejected = false;
    try { (void)composite({red,far,mc,near,none}); }
    catch (const std::invalid_argument&) { rejected = true; }
    REQUIRE(rejected);
}

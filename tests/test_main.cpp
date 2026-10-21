#include "prismforge/filters.hpp"
#include "prismforge/ppm.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
int failures = 0;
int checks = 0;
void check(bool ok, const std::string& name) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << name << '\n'; }
}
template<class F> void throws(F function, const std::string& name) {
    try { function(); check(false, name); } catch (const std::exception&) { check(true, name); }
}
void tests() {
    using namespace prismforge;
    throws([] { Image image(0, 1); (void)image; }, "zero width rejected");
    throws([] { Image image(1, 0); (void)image; }, "zero height rejected");
    throws([] { Image image(std::numeric_limits<std::size_t>::max(), 2); (void)image; }, "dimension overflow rejected");
    Image flat(3, 3, {40, 80, 120});
    check(gaussian_blur(flat, 0, 1.0) == flat, "zero-radius blur is identity");
    check(gaussian_blur(flat, 2, 1.0) == flat, "blur preserves constant image incl. border");
    const auto kernel = gaussian_kernel(3, 1.5);
    double sum = 0; for (const auto coefficient : kernel) sum += coefficient;
    check(kernel.size() == 7 && std::abs(sum - 1.0) < 1e-12, "Gaussian kernel normalized");
    throws([] { (void)gaussian_kernel(1, 0); }, "zero sigma rejected");
    throws([] { (void)gaussian_kernel(129, 1); }, "oversize radius rejected");
    throws([] { (void)gaussian_kernel(1, std::numeric_limits<double>::infinity()); }, "infinite sigma rejected");
    check(gaussian_kernel(2, 1e-200) == std::vector<double>({0,0,1,0,0}), "tiny positive sigma stays numerically stable");
    const auto constant_edges = sobel_edges(flat);
    bool all_zero = true; for (auto p : constant_edges.pixels()) all_zero = all_zero && p == Pixel{0,0,0};
    check(all_zero, "Sobel constant pattern yields zero including replicated border");
    Image step(5, 5, {0,0,0});
    for (std::size_t y = 0; y < 5; ++y) for (std::size_t x = 2; x < 5; ++x) step.at(x,y) = {255,255,255};
    const auto edges = sobel_edges(step);
    check(edges.at(2,2) == Pixel{255,255,255} && edges.at(0,2) == Pixel{0,0,0} && edges.at(4,2) == Pixel{0,0,0}, "Sobel known step result and replicated border");
    check(color_adjust(flat, 0, 1, 1) == flat, "neutral color adjustment is identity");
    const auto bright = color_adjust(Image(1,1,{250,120,3}), 20, 1, 1).at(0,0);
    check(bright == Pixel{255,140,23}, "brightness clamps high channel safely");
    check(color_adjust(Image(1,1,{255,0,0}), 0, 1, 0).at(0,0).r ==
          color_adjust(Image(1,1,{255,0,0}), 0, 1, 0).at(0,0).g, "zero saturation gives gray");
    throws([&] { (void)color_adjust(flat, 0, -1, 1); }, "negative contrast rejected");
    throws([&] { (void)color_adjust(flat, 0, 1, -1); }, "negative saturation rejected");
    throws([&] { (void)color_adjust(flat, std::numeric_limits<double>::quiet_NaN(), 1, 1); }, "nonfinite color value rejected");
    const auto temp = std::filesystem::temp_directory_path() / "prismforge-roundtrip.ppm";
    save_ppm(step, temp);
    check(load_ppm(temp) == step, "P6 PPM roundtrip");
    std::filesystem::remove(temp);
    throws([] { (void)load_ppm("prismforge-file-that-does-not-exist.ppm"); }, "failed input reported");
    const auto bad_output = std::filesystem::temp_directory_path() / "prismforge-missing-parent" / "out.ppm";
    throws([&] { save_ppm(Image(1,1), bad_output); }, "failed output reported");
    check(gaussian_blur(step, 2, 1.2) == gaussian_blur(step, 2, 1.2), "repeated processing deterministic");
}
}
int main() {
    tests();
    if (failures) { std::cerr << failures << " test(s) failed\n"; return 1; }
    std::cout << checks << " checks passed\n"; return 0;
}

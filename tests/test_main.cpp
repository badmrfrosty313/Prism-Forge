#include "prismforge/codec.hpp"
#include "prismforge/filters.hpp"
#include "prismforge/pipeline.hpp"
#include "prismforge/ppm.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
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
    Image bounded(1, 1);
    throws([&] { (void)bounded.at(1, 0); }, "mutable pixel bounds checked");
    const Image& const_bounded = bounded;
    throws([&] { (void)const_bounded.at(0, 1); }, "const pixel bounds checked");
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

    const auto fixture_dir = std::filesystem::temp_directory_path() / "prismforge-ppm-fixtures";
    std::filesystem::create_directories(fixture_dir);
    const auto write_fixture = [&](const std::string& name, const std::string& bytes) {
        const auto path = fixture_dir / name;
        std::ofstream file(path, std::ios::binary);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        return path;
    };
    const auto p3_path = write_fixture("commented-ascii.ppm", "P3\n# sample\n2 1\n255\n255 0 1 4 5 6\n");
    const auto p3 = load_ppm(p3_path);
    check(p3.width() == 2 && p3.height() == 1 && p3.at(0,0) == Pixel{255,0,1} && p3.at(1,0) == Pixel{4,5,6}, "P3 input parses comments and RGB values");
    const std::string crlf_header = "P6\r\n1 1\r\n255\r\n";
    const auto crlf_path = write_fixture("crlf.ppm", crlf_header + std::string("\x0a\x20\xff", 3));
    check(load_ppm(crlf_path).at(0,0) == Pixel{10,32,255}, "P6 CRLF separator preserves whitespace-valued first pixel");
    const auto malformed_path = [&](const std::string& name, const std::string& contents) {
        return write_fixture(name, contents);
    };
    throws([&] { (void)load_ppm(malformed_path("bad-magic.ppm", "P9\n1 1\n255\n")); }, "unsupported PPM magic rejected");
    throws([&] { (void)load_ppm(malformed_path("missing-header.ppm", "P6\n1\n")); }, "truncated PPM header rejected");
    throws([&] { (void)load_ppm(malformed_path("negative.ppm", "P3\n-1 1\n255\n")); }, "negative PPM dimension rejected");
    throws([&] { (void)load_ppm(malformed_path("zero.ppm", "P3\n0 1\n255\n")); }, "zero PPM dimension rejected");
    throws([&] { (void)load_ppm(malformed_path("huge.ppm", "P6\n4097 4097\n255\n")); }, "oversized pixel budget rejected before allocation");
    throws([&] { (void)load_ppm(malformed_path("wide.ppm", "P6\n16385 1\n255\n")); }, "oversized single dimension rejected");
    throws([&] { (void)load_ppm(malformed_path("maxval.ppm", "P3\n1 1\n100\n0 0 0\n")); }, "unsupported max value rejected");
    throws([&] { (void)load_ppm(malformed_path("sample-overflow.ppm", "P3\n1 1\n255\n256 0 0\n")); }, "P3 sample over 255 rejected");
    throws([&] { (void)load_ppm(malformed_path("sample-negative.ppm", "P3\n1 1\n255\n-1 0 0\n")); }, "negative P3 sample rejected");
    throws([&] { (void)load_ppm(malformed_path("truncated.ppm", "P6\n2 1\n255\n\x01\x02\x03")); }, "truncated P6 raster rejected");
    const auto oversized_ppm = malformed_path("oversized-input.ppm", "P3\n1 1\n255\n0 0 0\n");
    std::filesystem::resize_file(oversized_ppm, max_encoded_input_bytes + 1);
    throws([&] { (void)load_ppm(oversized_ppm); }, "oversized PPM file rejected before parsing");
    std::filesystem::remove_all(fixture_dir);
    check(gaussian_blur(step, 2, 1.2) == gaussian_blur(step, 2, 1.2), "repeated processing deterministic");

    std::filesystem::create_directories(fixture_dir);
    const Image rgba(1, 1, {100, 150, 200, 7});
    check(grayscale(rgba).at(0,0) == Pixel{141,141,141,7}, "grayscale uses Rec. 601 luma and preserves alpha");
    check(invert(rgba).at(0,0) == Pixel{155,105,55,7}, "invert changes RGB and preserves alpha");
    check(color_adjust(rgba, 10, 1, 1).at(0,0).a == 7, "color adjustment preserves alpha");
    check(gaussian_blur(rgba, 1, 1).at(0,0).a == 7 && sobel_edges(rgba).at(0,0).a == 7,
          "blur and Sobel preserve center alpha");
    throws([&] { save_ppm(rgba, fixture_dir / "alpha-loss.ppm"); }, "PPM output rejects alpha loss");
    throws([&] { (void)sharpen(rgba, -0.1); }, "negative sharpen amount rejected");
    check(sharpen(Image(3,3,{0,0,0}), 1.0) == Image(3,3,{0,0,0}), "sharpen leaves constant image unchanged");
    Image impulse(3,3,{0,0,0}); impulse.at(1,1) = {200,100,50,33};
    check(sharpen(impulse,1).at(1,1) == Pixel{255,255,250,33}, "sharpen uses documented five-point kernel and preserves alpha");
    Image pair(2,1); pair.at(0,0) = {0,0,0,0}; pair.at(1,0) = {100,100,100,255};
    const auto nearest = resize_nearest(pair,4,1);
    check(nearest.at(0,0) == pair.at(0,0) && nearest.at(1,0) == pair.at(0,0) &&
          nearest.at(2,0) == pair.at(1,0) && nearest.at(3,0) == pair.at(1,0), "nearest resize maps source pixel centers");
    const auto bilinear = resize_bilinear(pair,3,1);
    check(bilinear.at(0,0) == pair.at(0,0) && bilinear.at(1,0) == Pixel{50,50,50,128} &&
          bilinear.at(2,0) == pair.at(1,0), "bilinear resize interpolates RGBA at pixel centers");
    throws([&] { (void)resize_nearest(pair,0,2); }, "zero resize dimension rejected");
    throws([&] { (void)resize_bilinear(pair, max_image_pixels + 1, 1); }, "oversized resize rejected before allocation");
    const auto blur_then_sobel = apply_pipeline(step, {parse_filter_spec("blur:radius=1,sigma=1"), parse_filter_spec("sobel")});
    const auto sobel_then_blur = apply_pipeline(step, {parse_filter_spec("sobel"), parse_filter_spec("blur:radius=1,sigma=1")});
    check(!(blur_then_sobel == sobel_then_blur), "pipeline executes filters in supplied order");
    check(apply_pipeline(rgba, {}) == rgba, "empty pipeline is an identity");
    throws([] { (void)parse_filter_spec("unknown"); }, "unknown pipeline filter rejected");
    throws([] { (void)parse_filter_spec("blur:radius=-1"); }, "negative pipeline radius rejected");
    throws([] { (void)parse_filter_spec("grayscale:brightness=1"); }, "irrelevant pipeline parameter rejected");
    throws([] { (void)parse_filter_spec("resize-nearest:width=2"); }, "incomplete resize parameters rejected");

    const auto codec_dir = fixture_dir;
    std::filesystem::create_directories(codec_dir);
    const auto png_path = codec_dir / "rgba.png";
    save_image(rgba, png_path);
    check(load_image(png_path) == rgba, "PNG roundtrip preserves exact RGBA pixel");
    const auto renamed_png = codec_dir / "content-detected.bin";
    std::filesystem::rename(png_path, renamed_png);
    check(load_image(renamed_png) == rgba, "PNG input format detected from content not extension");
    const auto alpha_jpeg = codec_dir / "alpha.jpg";
    throws([&] { save_image(rgba, alpha_jpeg); }, "JPEG output rejects silent alpha loss");
    const auto opaque_jpeg = codec_dir / "opaque.jpg";
    const Image jpeg_source(24,24,{80,120,200});
    save_image(jpeg_source, opaque_jpeg, 92);
    const auto jpeg_result = load_image(opaque_jpeg);
    check(jpeg_result.width() == 24 && jpeg_result.height() == 24 && jpeg_result.at(12,12).a == 255 &&
          std::abs(static_cast<int>(jpeg_result.at(12,12).r) - 80) <= 8 &&
          std::abs(static_cast<int>(jpeg_result.at(12,12).g) - 120) <= 8 &&
          std::abs(static_cast<int>(jpeg_result.at(12,12).b) - 200) <= 8, "JPEG roundtrip stays within lossy tolerance");
    const auto converted_ppm = codec_dir / "converted.ppm";
    save_image(jpeg_result, converted_ppm);
    check(load_image(converted_ppm) == jpeg_result, "JPEG to PPM conversion roundtrip");
    throws([&] { save_image(jpeg_source, codec_dir / "bad-quality.jpg", 0); }, "invalid JPEG quality rejected");
    throws([&] { (void)load_image(malformed_path("not-an-image.bin", "not an image")); }, "unsupported codec input rejected");
    throws([&] { (void)load_image(malformed_path("broken.png", std::string("\x89PNG\r\n\x1a\n",8) + "broken")); }, "corrupt PNG rejected");
    throws([&] { (void)load_image(malformed_path("broken.jpg", std::string("\xff\xd8\xff",3) + "broken")); }, "corrupt JPEG rejected");
    const auto oversized_codec = malformed_path("oversized.png", std::string("oversized"));
    std::filesystem::resize_file(oversized_codec, max_encoded_input_bytes + 1);
    throws([&] { (void)load_image(oversized_codec); }, "oversized encoded input rejected before allocation");
    std::filesystem::remove_all(codec_dir);
}
}
int main() {
    tests();
    if (failures) { std::cerr << failures << " test(s) failed\n"; return 1; }
    std::cout << checks << " checks passed\n"; return 0;
}

#include "prismforge/filters.hpp"
#include "prismforge/pipeline.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
volatile std::uint64_t checksum_sink = 0;

std::uint64_t checksum(const prismforge::Image& image) {
    // FNV-1a over dimensions and row-major RGBA bytes.
    std::uint64_t value = 14695981039346656037ULL;
    const auto add = [&value](std::uint8_t byte) { value ^= byte; value *= 1099511628211ULL; };
    for (unsigned shift = 0; shift < 64; shift += 8) {
        add(static_cast<std::uint8_t>((static_cast<std::uint64_t>(image.width()) >> shift) & 0xffU));
        add(static_cast<std::uint8_t>((static_cast<std::uint64_t>(image.height()) >> shift) & 0xffU));
    }
    for (const auto pixel : image.pixels()) { add(pixel.r); add(pixel.g); add(pixel.b); add(pixel.a); }
    return value;
}

std::size_t parse_dimension(const char* value, const char* name) {
    const auto parsed = std::stoull(value);
    if (parsed == 0 || parsed > 16384) throw std::invalid_argument(std::string(name) + " must be in 1..16384");
    return static_cast<std::size_t>(parsed);
}
unsigned parse_repetitions(const char* value) {
    const auto parsed = std::stoul(value);
    if (parsed == 0 || parsed > 10000) throw std::invalid_argument("repetitions must be in 1..10000");
    return static_cast<unsigned>(parsed);
}
}

int main(int argc, char** argv) {
    try {
        const std::size_t width = argc > 1 ? parse_dimension(argv[1], "width") : 256;
        const std::size_t height = argc > 2 ? parse_dimension(argv[2], "height") : 256;
        const unsigned repetitions = argc > 3 ? parse_repetitions(argv[3]) : 5;
        if (argc > 4 || width > prismforge::max_image_pixels / height)
            throw std::invalid_argument("image exceeds the 16 Mi pixel benchmark limit or has extra arguments");
        prismforge::Image image(width, height);
        for (std::size_t y = 0; y < height; ++y) for (std::size_t x = 0; x < width; ++x)
            image.at(x, y) = {static_cast<std::uint8_t>((x * 17 + y * 3) % 256),
                              static_cast<std::uint8_t>((x * 5 + y * 13) % 256),
                              static_cast<std::uint8_t>((x * 7 + y * 11) % 256)};

        std::cout << "filter,width,height,repetitions,parameters,compiler,build,total_ms,mean_ms,checksum\n";
        const auto run = [&](const std::string& name, const std::string& parameters, auto filter) {
            double elapsed_ms = 0.0;
            std::uint64_t rolling_checksum = 0;
            for (unsigned i = 0; i < repetitions; ++i) {
                const auto start = std::chrono::steady_clock::now();
                const auto output = filter(image);
                const auto stop = std::chrono::steady_clock::now();
                elapsed_ms += std::chrono::duration<double, std::milli>(stop - start).count();
                const auto output_checksum = checksum(output); // outside the timed interval
                rolling_checksum = (rolling_checksum * 0x9e3779b185ebca87ULL) ^ output_checksum;
                checksum_sink = rolling_checksum; // observable output prevents dead-code elimination
            }
#if defined(_MSC_VER)
            const char* compiler = "MSVC";
#elif defined(__clang__)
            const char* compiler = "Clang";
#elif defined(__GNUC__)
            const char* compiler = "GCC";
#else
            const char* compiler = "unknown";
#endif
#ifdef NDEBUG
            const char* build = "Release";
#else
            const char* build = "Debug";
#endif
            std::cout << name << ',' << width << ',' << height << ',' << repetitions << ",\"" << parameters << "\","
                      << compiler << ',' << build << ',' << elapsed_ms << ',' << elapsed_ms / repetitions << ','
                      << rolling_checksum << '\n';
        };
        run("gaussian_blur", "radius=3;sigma=1.5", [](const auto& im) { return prismforge::gaussian_blur(im, 3, 1.5); });
        run("sobel_edges", "", [](const auto& im) { return prismforge::sobel_edges(im); });
        run("color_adjust", "brightness=8;contrast=1.1;saturation=1.2", [](const auto& im) { return prismforge::color_adjust(im, 8, 1.1, 1.2); });
        run("grayscale", "", [](const auto& im) { return prismforge::grayscale(im); });
        run("sharpen", "amount=0.8", [](const auto& im) { return prismforge::sharpen(im, 0.8); });
        run("resize_bilinear", "width=128;height=96", [](const auto& im) { return prismforge::resize_bilinear(im, 128, 96); });
        const std::vector<prismforge::FilterSpec> pipeline{
            prismforge::parse_filter_spec("blur:radius=3,sigma=1.5"),
            prismforge::parse_filter_spec("grayscale"),
            prismforge::parse_filter_spec("sharpen:amount=0.8")};
        run("pipeline", "blur(radius=3,sigma=1.5)+grayscale+sharpen(amount=0.8)",
            [&pipeline](const auto& im) { return prismforge::apply_pipeline(im, pipeline); });
        return checksum_sink == 0 ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "benchmark: " << error.what() << '\n';
        return 2;
    }
}

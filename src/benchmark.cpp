#include "prismforge/filters.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
volatile std::uint64_t checksum_sink = 0;

std::uint64_t checksum(const prismforge::Image& image) {
    // FNV-1a over dimensions and row-major RGB bytes.
    std::uint64_t value = 14695981039346656037ULL;
    const auto add = [&value](std::uint8_t byte) {
        value ^= byte;
        value *= 1099511628211ULL;
    };
    for (unsigned shift = 0; shift < 64; shift += 8) {
        add(static_cast<std::uint8_t>((static_cast<std::uint64_t>(image.width()) >> shift) & 0xffU));
        add(static_cast<std::uint8_t>((static_cast<std::uint64_t>(image.height()) >> shift) & 0xffU));
    }
    for (const auto pixel : image.pixels()) { add(pixel.r); add(pixel.g); add(pixel.b); }
    return value;
}

std::size_t parse_dimension(const char* value, const char* name) {
    const auto parsed = std::stoull(value);
    if (parsed == 0 || parsed > 16'384) throw std::invalid_argument(std::string(name) + " must be in 1..16384");
    return static_cast<std::size_t>(parsed);
}
unsigned parse_repetitions(const char* value) {
    const auto parsed = std::stoul(value);
    if (parsed == 0 || parsed > 10'000) throw std::invalid_argument("repetitions must be in 1..10000");
    return static_cast<unsigned>(parsed);
}
}

int main(int argc, char** argv) {
    try {
    const std::size_t width = argc > 1 ? parse_dimension(argv[1], "width") : 256;
    const std::size_t height = argc > 2 ? parse_dimension(argv[2], "height") : 256;
    const unsigned repetitions = argc > 3 ? parse_repetitions(argv[3]) : 5;
    constexpr std::size_t max_pixels = 16U * 1024U * 1024U;
    if (argc > 4 || width > max_pixels / height) throw std::invalid_argument("image exceeds the 16 Mi pixel benchmark limit or has extra arguments");
    prismforge::Image image(width, height);
    for (std::size_t y = 0; y < height; ++y) for (std::size_t x = 0; x < width; ++x)
        image.at(x,y) = {static_cast<std::uint8_t>((x * 17 + y * 3) % 256), static_cast<std::uint8_t>((x * 5 + y * 13) % 256), static_cast<std::uint8_t>((x * 7 + y * 11) % 256)};
    const auto run = [&](const char* name, auto filter) {
        double elapsed_ms = 0.0;
        std::uint64_t rolling_checksum = 0;
        for (unsigned i = 0; i < repetitions; ++i) {
            const auto start = std::chrono::steady_clock::now();
            const auto output = filter(image);
            const auto stop = std::chrono::steady_clock::now();
            elapsed_ms += std::chrono::duration<double, std::milli>(stop - start).count();
            // Hashing is outside the timed interval. The volatile sink makes every
            // produced image observably consumed, including across repeated calls.
            const auto output_checksum = checksum(output);
            rolling_checksum = (rolling_checksum * 0x9e3779b185ebca87ULL) ^ output_checksum;
            checksum_sink = rolling_checksum;
        }
        std::cout << name << "," << width << "x" << height << "," << repetitions
                  << "," << elapsed_ms << " ms filter time," << elapsed_ms / repetitions
                  << " ms/filter run,checksum=" << rolling_checksum << '\n';
    };
    std::cout << "compiler=" <<
#if defined(_MSC_VER)
                 "MSVC " << _MSC_VER
#elif defined(__clang__)
                 "Clang " << __clang_version__
#elif defined(__GNUC__)
                 "GCC " << __VERSION__
#else
                 "unknown"
#endif
              << ",build=" <<
#ifdef NDEBUG
                 "Release"
#else
                 "Debug"
#endif
              << ",environment=local process\n";
    run("gaussian_blur(radius=3,sigma=1.5)", [](const auto& im) { return prismforge::gaussian_blur(im, 3, 1.5); });
    run("sobel_edges", [](const auto& im) { return prismforge::sobel_edges(im); });
    run("color_adjust(brightness=8,contrast=1.1,saturation=1.2)", [](const auto& im) { return prismforge::color_adjust(im, 8, 1.1, 1.2); });
    std::cout << "observable_checksum_sink=" << checksum_sink << '\n';
    return 0;
    } catch (const std::exception& error) { std::cerr << "benchmark: " << error.what() << '\n'; return 2; }
}

#include "prismforge/filters.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    const std::size_t width = argc > 1 ? static_cast<std::size_t>(std::stoul(argv[1])) : 256;
    const std::size_t height = argc > 2 ? static_cast<std::size_t>(std::stoul(argv[2])) : 256;
    const unsigned repetitions = argc > 3 ? static_cast<unsigned>(std::stoul(argv[3])) : 5;
    if (width == 0 || height == 0 || repetitions == 0) { std::cerr << "dimensions and repetitions must be positive\n"; return 2; }
    prismforge::Image image(width, height);
    for (std::size_t y = 0; y < height; ++y) for (std::size_t x = 0; x < width; ++x)
        image.at(x,y) = {static_cast<std::uint8_t>((x * 17 + y * 3) % 256), static_cast<std::uint8_t>((x * 5 + y * 13) % 256), static_cast<std::uint8_t>((x * 7 + y * 11) % 256)};
    const auto run = [&](const char* name, auto filter) {
        const auto start = std::chrono::steady_clock::now();
        for (unsigned i = 0; i < repetitions; ++i) { auto output = filter(image); (void)output; }
        const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        std::cout << name << "," << width << "x" << height << "," << repetitions << "," << elapsed << " ms total," << elapsed / repetitions << " ms/run\n";
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
}

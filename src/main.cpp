#include "prismforge/filters.hpp"
#include "prismforge/ppm.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void usage() {
    std::cerr << "Usage: prismforge --input in.ppm --output out.ppm --effect blur|sobel|color [options]\n"
              << "  blur: --radius N --sigma S (defaults 3, 1.5)\n"
              << "  color: --brightness N --contrast N --saturation N (defaults 0, 1, 1)\n";
}
double real(const std::string& value, const char* name) {
    std::size_t used = 0; const auto result = std::stod(value, &used);
    if (used != value.size()) throw std::invalid_argument(std::string("invalid ") + name);
    return result;
}
unsigned integer(const std::string& value, const char* name) {
    std::size_t used = 0; const auto result = std::stoul(value, &used);
    if (used != value.size()) throw std::invalid_argument(std::string("invalid ") + name);
    if (result > std::numeric_limits<unsigned>::max()) throw std::invalid_argument(std::string(name) + " is too large");
    return static_cast<unsigned>(result);
}
}

int main(int argc, char** argv) {
    try {
        std::string input, output, effect;
        unsigned radius = 3; double sigma = 1.5, brightness = 0, contrast = 1, saturation = 1;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--help" || key == "-h") { usage(); return 0; }
            if (i + 1 >= argc) throw std::invalid_argument("missing value for " + key);
            const std::string value = argv[++i];
            if (key == "--input") input = value;
            else if (key == "--output") output = value;
            else if (key == "--effect") effect = value;
            else if (key == "--radius") radius = integer(value, "radius");
            else if (key == "--sigma") sigma = real(value, "sigma");
            else if (key == "--brightness") brightness = real(value, "brightness");
            else if (key == "--contrast") contrast = real(value, "contrast");
            else if (key == "--saturation") saturation = real(value, "saturation");
            else throw std::invalid_argument("unknown option: " + key);
        }
        if (input.empty() || output.empty() || effect.empty()) throw std::invalid_argument("input, output, and effect are required");
        const auto image = prismforge::load_ppm(input);
        prismforge::Image result = image;
        if (effect == "blur") result = prismforge::gaussian_blur(image, radius, sigma);
        else if (effect == "sobel") result = prismforge::sobel_edges(image);
        else if (effect == "color") result = prismforge::color_adjust(image, brightness, contrast, saturation);
        else throw std::invalid_argument("unknown effect: " + effect);
        prismforge::save_ppm(result, output);
        std::cout << "Processed " << image.width() << 'x' << image.height() << " image with " << effect << " -> " << output << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "prismforge: " << error.what() << '\n'; usage(); return 2;
    }
}

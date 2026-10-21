#include "prismforge/ppm.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace prismforge {
namespace {
std::string token(std::istream& stream) {
    std::string value;
    while (stream >> std::ws) {
        if (stream.peek() != '#') break;
        stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    if (!(stream >> value)) throw std::runtime_error("unexpected end of PPM header/data");
    return value;
}
std::size_t number(const std::string& text) {
    std::size_t consumed = 0;
    const auto result = std::stoull(text, &consumed);
    if (consumed != text.size()) throw std::runtime_error("invalid number in PPM file");
    return static_cast<std::size_t>(result);
}
}

Image load_ppm(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open input file: " + path.string());
    const auto magic = token(input);
    if (magic != "P6" && magic != "P3") throw std::runtime_error("unsupported PPM format (expected P6 or P3)");
    const auto width = number(token(input));
    const auto height = number(token(input));
    const auto max_value = number(token(input));
    if (max_value != 255) throw std::runtime_error("PPM max value must be 255");
    Image image(width, height);
    if (magic == "P6") {
        input.get();
        for (std::size_t y = 0; y < height; ++y) for (std::size_t x = 0; x < width; ++x) {
            char rgb[3];
            if (!input.read(rgb, 3)) throw std::runtime_error("truncated PPM pixel data");
            image.at(x,y) = {static_cast<std::uint8_t>(static_cast<unsigned char>(rgb[0])),
                             static_cast<std::uint8_t>(static_cast<unsigned char>(rgb[1])),
                             static_cast<std::uint8_t>(static_cast<unsigned char>(rgb[2]))};
        }
    } else {
        for (std::size_t y = 0; y < height; ++y) for (std::size_t x = 0; x < width; ++x) {
            const auto r = number(token(input)), g = number(token(input)), b = number(token(input));
            if (r > 255 || g > 255 || b > 255) throw std::runtime_error("PPM sample outside 0..255");
            image.at(x,y) = {static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g), static_cast<std::uint8_t>(b)};
        }
    }
    return image;
}

void save_ppm(const Image& image, const std::filesystem::path& path) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot open output file: " + path.string());
    output << "P6\n" << image.width() << ' ' << image.height() << "\n255\n";
    for (const auto p : image.pixels()) {
        const char rgb[3]{static_cast<char>(p.r), static_cast<char>(p.g), static_cast<char>(p.b)};
        output.write(rgb, 3);
    }
    if (!output) throw std::runtime_error("failed while writing output file: " + path.string());
}
}

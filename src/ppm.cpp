#include "prismforge/ppm.hpp"

#include <fstream>
#include <charconv>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

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
    if (text.empty()) throw std::runtime_error("empty number in PPM file");
    for (const char raw_character : text) {
        const auto character = static_cast<unsigned char>(raw_character);
        if (!std::isdigit(character)) throw std::runtime_error("invalid nonnegative integer in PPM file");
    }
    std::size_t result = 0;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        throw std::runtime_error("integer outside supported range in PPM file");
    return result;
}
void check_dimensions(std::size_t width, std::size_t height) {
    if (width == 0 || height == 0) throw std::runtime_error("PPM dimensions must be nonzero");
    if (width > ppm_max_dimension || height > ppm_max_dimension || width > ppm_max_pixels / height)
        throw std::runtime_error("PPM dimensions exceed the 16 Mi pixel safety limit");
}
void consume_binary_separator(std::istream& input) {
    const int first = input.get();
    if (first == std::char_traits<char>::eof() || !std::isspace(static_cast<unsigned char>(first)))
        throw std::runtime_error("missing whitespace separator before PPM pixel data");
    // CRLF is one logical header line ending; consume both bytes without skipping
    // arbitrary whitespace that could itself be the first binary pixel sample.
    if (first == '\r' && input.peek() == '\n') input.get();
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
    check_dimensions(width, height);
    Image image(width, height);
    if (magic == "P6") {
        consume_binary_separator(input);
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

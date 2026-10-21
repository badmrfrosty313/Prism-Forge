#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 16384
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STB_IMAGE_IMPLEMENTATION
#define STBIW_ONLY_JPEG
#define STBIW_ONLY_PNG
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "prismforge/codec.hpp"
#include "prismforge/ppm.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace prismforge {
namespace {
constexpr unsigned char png_signature[]{137, 80, 78, 71, 13, 10, 26, 10};
constexpr std::size_t max_input_bytes = max_encoded_input_bytes;
static_assert(sizeof(Pixel) == 4, "RGBA Pixel must be four packed bytes for stb PNG output");

bool has_prefix(const std::vector<unsigned char>& data, const unsigned char* prefix, std::size_t count) {
    return data.size() >= count && std::equal(prefix, prefix + count, data.begin());
}

std::vector<unsigned char> read_encoded_file(const std::filesystem::path& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error) throw std::runtime_error("cannot determine input file size: " + path.string());
    if (size == 0) throw std::runtime_error("input file is empty: " + path.string());
    if (size > max_input_bytes) throw std::runtime_error("encoded input exceeds the 64 MiB file-size limit");
    std::vector<unsigned char> data(static_cast<std::size_t>(size));
    std::ifstream input(path, std::ios::binary);
    if (!input || !input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size())))
        throw std::runtime_error("cannot read complete input file: " + path.string());
    return data;
}

void validate_dimensions(int width, int height) {
    if (width <= 0 || height <= 0) throw std::runtime_error("decoded image has invalid dimensions");
    const auto w = static_cast<std::size_t>(width);
    const auto h = static_cast<std::size_t>(height);
    if (w > max_image_dimension || h > max_image_dimension || w > max_image_pixels / h)
        throw std::runtime_error("decoded image exceeds the 16 Mi pixel safety limit");
}

void write_callback(void* context, void* data, int size) {
    auto& output = *static_cast<std::ostream*>(context);
    output.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
}

std::string lowercase_extension(const std::filesystem::path& path) {
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return extension;
}

bool has_transparency(const Image& image) {
    return std::any_of(image.pixels().begin(), image.pixels().end(), [](Pixel p) { return p.a != 255; });
}
}

Image load_image(const std::filesystem::path& path) {
    const auto data = read_encoded_file(path);
    if (data.size() >= 2 && data[0] == 'P' && (data[1] == '3' || data[1] == '6')) return load_ppm(path);

    const bool png = has_prefix(data, png_signature, sizeof(png_signature));
    const bool jpeg = data.size() >= 3 && data[0] == 0xff && data[1] == 0xd8 && data[2] == 0xff;
    if (!png && !jpeg) throw std::runtime_error("unsupported image content; expected PNG, JPEG, P3, or P6 PPM");

    int width = 0, height = 0, source_channels = 0;
    if (stbi_info_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &source_channels) == 0)
        throw std::runtime_error(std::string("invalid image header: ") + stbi_failure_reason());
    if (source_channels < 1 || source_channels > 4)
        throw std::runtime_error("input image has an unsupported channel count");
    validate_dimensions(width, height);
    int decoded_width = 0, decoded_height = 0, decoded_channels = 0;
    unsigned char* decoded = stbi_load_from_memory(data.data(), static_cast<int>(data.size()),
                                                   &decoded_width, &decoded_height, &decoded_channels,
                                                   STBI_rgb_alpha);
    if (decoded == nullptr) throw std::runtime_error(std::string("image decode failed: ") + stbi_failure_reason());
    try {
        validate_dimensions(decoded_width, decoded_height);
        if (decoded_width != width || decoded_height != height)
            throw std::runtime_error("image dimensions changed between header check and decode");
        if (decoded_channels < 1 || decoded_channels > 4)
            throw std::runtime_error("decoded image has an unsupported channel count");
        Image image(static_cast<std::size_t>(width), static_cast<std::size_t>(height));
        auto* rgba = decoded;
        for (std::size_t y = 0; y < image.height(); ++y) for (std::size_t x = 0; x < image.width(); ++x) {
            image.at(x, y) = {rgba[0], rgba[1], rgba[2], rgba[3]};
            rgba += 4;
        }
        stbi_image_free(decoded);
        return image;
    } catch (...) {
        stbi_image_free(decoded);
        throw;
    }
}

void save_image(const Image& image, const std::filesystem::path& path, int jpeg_quality) {
    const auto extension = lowercase_extension(path);
    if (extension == ".ppm") { save_ppm(image, path); return; }
    if (extension != ".png" && extension != ".jpg" && extension != ".jpeg")
        throw std::runtime_error("unsupported output extension; use .png, .jpg, .jpeg, or .ppm");
    if ((extension == ".jpg" || extension == ".jpeg") && (jpeg_quality < 1 || jpeg_quality > 100))
        throw std::invalid_argument("JPEG quality must be in 1..100");
    if ((extension == ".jpg" || extension == ".jpeg") && has_transparency(image))
        throw std::runtime_error("JPEG has no alpha channel; convert transparency explicitly before saving");
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("cannot open output file: " + path.string());
    if (extension == ".png") {
        const auto stride = static_cast<int>(image.width() * sizeof(Pixel));
        if (stbi_write_png_to_func(write_callback, &output, static_cast<int>(image.width()),
                                   static_cast<int>(image.height()), 4, image.pixels().data(), stride) == 0)
            throw std::runtime_error("PNG encoding failed");
    } else if (extension == ".jpg" || extension == ".jpeg") {
        std::vector<unsigned char> rgb(image.pixels().size() * 3U);
        std::size_t offset = 0;
        for (const auto pixel : image.pixels()) {
            rgb[offset++] = pixel.r; rgb[offset++] = pixel.g; rgb[offset++] = pixel.b;
        }
        if (stbi_write_jpg_to_func(write_callback, &output, static_cast<int>(image.width()),
                                   static_cast<int>(image.height()), 3, rgb.data(), jpeg_quality) == 0)
            throw std::runtime_error("JPEG encoding failed");
    }
    output.flush();
    if (!output) throw std::runtime_error("failed while writing output file: " + path.string());
}

bool is_supported_image_extension(const std::filesystem::path& path) {
    const auto extension = lowercase_extension(path);
    return extension == ".ppm" || extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}

} // namespace prismforge

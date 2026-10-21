#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace prismforge {

inline constexpr std::size_t max_image_dimension = 16'384;
inline constexpr std::size_t max_image_pixels = 16U * 1024U * 1024U;
inline constexpr std::size_t max_encoded_input_bytes = 64U * 1024U * 1024U;

struct Pixel {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};
    bool operator==(const Pixel& other) const noexcept;
    bool operator!=(const Pixel& other) const noexcept { return !(*this == other); }
};

class Image {
public:
    Image(std::size_t width, std::size_t height, Pixel fill = {});
    std::size_t width() const noexcept { return width_; }
    std::size_t height() const noexcept { return height_; }
    bool empty() const noexcept { return pixels_.empty(); }
    Pixel& at(std::size_t x, std::size_t y);
    const Pixel& at(std::size_t x, std::size_t y) const;
    const std::vector<Pixel>& pixels() const noexcept { return pixels_; }
    bool operator==(const Image& other) const noexcept;
private:
    std::size_t width_;
    std::size_t height_;
    std::vector<Pixel> pixels_;
};

} // namespace prismforge

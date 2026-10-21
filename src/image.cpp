#include "prismforge/image.hpp"

#include <limits>
#include <stdexcept>

namespace prismforge {

bool Pixel::operator==(const Pixel& other) const noexcept {
    return r == other.r && g == other.g && b == other.b;
}

Image::Image(std::size_t width, std::size_t height, Pixel fill) : width_(width), height_(height) {
    if (width == 0 || height == 0) throw std::invalid_argument("image dimensions must be nonzero");
    if (width > std::numeric_limits<std::size_t>::max() / height)
        throw std::length_error("image dimensions overflow");
    pixels_.assign(width * height, fill);
}

Pixel& Image::at(std::size_t x, std::size_t y) {
    if (x >= width_ || y >= height_) throw std::out_of_range("pixel coordinate outside image");
    return pixels_[y * width_ + x];
}

const Pixel& Image::at(std::size_t x, std::size_t y) const {
    if (x >= width_ || y >= height_) throw std::out_of_range("pixel coordinate outside image");
    return pixels_[y * width_ + x];
}

bool Image::operator==(const Image& other) const noexcept {
    return width_ == other.width_ && height_ == other.height_ && pixels_ == other.pixels_;
}

} // namespace prismforge

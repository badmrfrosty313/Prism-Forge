#pragma once

#include "prismforge/image.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

namespace prismforge {

enum class FilterKind { blur, sobel, color, grayscale, sharpen, invert, resize_nearest, resize_bilinear };

struct FilterSpec {
    FilterKind kind{FilterKind::grayscale};
    unsigned radius{3};
    double sigma{1.5};
    double brightness{0.0};
    double contrast{1.0};
    double saturation{1.0};
    double amount{1.0};
    std::size_t width{0};
    std::size_t height{0};
};

FilterSpec parse_filter_spec(std::string_view text);
Image apply_filter(const Image& image, const FilterSpec& filter);
Image apply_pipeline(Image image, const std::vector<FilterSpec>& filters);

} // namespace prismforge

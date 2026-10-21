#include "prismforge/pipeline.hpp"

#include "prismforge/filters.hpp"

#include <charconv>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>

namespace prismforge {
namespace {
std::pair<std::string, std::map<std::string, std::string>> split_filter(std::string_view text) {
    const auto colon = text.find(':');
    const std::string name(text.substr(0, colon));
    if (name.empty()) throw std::invalid_argument("filter name cannot be empty");
    std::map<std::string, std::string> parameters;
    if (colon == std::string_view::npos) return {name, parameters};
    auto rest = text.substr(colon + 1);
    if (rest.empty()) throw std::invalid_argument("filter parameter list cannot be empty");
    while (!rest.empty()) {
        const auto comma = rest.find(',');
        const auto item = rest.substr(0, comma);
        const auto equals = item.find('=');
        if (equals == std::string_view::npos || equals == 0 || equals + 1 == item.size())
            throw std::invalid_argument("filter parameters must use key=value syntax");
        std::string key(item.substr(0, equals)), value(item.substr(equals + 1));
        if (!parameters.emplace(std::move(key), std::move(value)).second)
            throw std::invalid_argument("duplicate filter parameter");
        if (comma == std::string_view::npos) break;
        rest.remove_prefix(comma + 1);
        if (rest.empty()) throw std::invalid_argument("filter parameter list cannot end with a comma");
    }
    return {name, parameters};
}

std::size_t parse_size(const std::string& value, const char* name) {
    std::size_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || result == 0)
        throw std::invalid_argument(std::string(name) + " must be a positive integer");
    return result;
}
unsigned parse_unsigned(const std::string& value, const char* name) {
    unsigned result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::invalid_argument(std::string(name) + " must be a nonnegative integer");
    return result;
}
double parse_real(const std::string& value, const char* name) {
    std::size_t used = 0;
    double result = 0.0;
    try { result = std::stod(value, &used); }
    catch (const std::exception&) { throw std::invalid_argument(std::string(name) + " must be numeric"); }
    if (used != value.size() || !std::isfinite(result))
        throw std::invalid_argument(std::string(name) + " must be finite and numeric");
    return result;
}

template<class T, class F>
void assign_if_present(const std::map<std::string, std::string>& values, const char* key, T& output, F parse) {
    const auto found = values.find(key);
    if (found != values.end()) output = parse(found->second, key);
}
void require_only(const std::map<std::string, std::string>& values,
                  std::initializer_list<const char*> allowed, const std::string& filter) {
    for (const auto& [key, value] : values) {
        (void)value;
        bool accepted = false;
        for (const auto* candidate : allowed) if (key == candidate) accepted = true;
        if (!accepted) throw std::invalid_argument("unknown parameter '" + key + "' for filter " + filter);
    }
}
}

FilterSpec parse_filter_spec(std::string_view text) {
    const auto [name, values] = split_filter(text);
    FilterSpec spec;
    if (name == "blur") {
        spec.kind = FilterKind::blur;
        require_only(values, {"radius", "sigma"}, name);
        assign_if_present(values, "radius", spec.radius, parse_unsigned);
        assign_if_present(values, "sigma", spec.sigma, parse_real);
    } else if (name == "sobel") {
        spec.kind = FilterKind::sobel; require_only(values, {}, name);
    } else if (name == "color") {
        spec.kind = FilterKind::color;
        require_only(values, {"brightness", "contrast", "saturation"}, name);
        assign_if_present(values, "brightness", spec.brightness, parse_real);
        assign_if_present(values, "contrast", spec.contrast, parse_real);
        assign_if_present(values, "saturation", spec.saturation, parse_real);
    } else if (name == "grayscale") {
        spec.kind = FilterKind::grayscale; require_only(values, {}, name);
    } else if (name == "sharpen") {
        spec.kind = FilterKind::sharpen;
        require_only(values, {"amount"}, name);
        assign_if_present(values, "amount", spec.amount, parse_real);
    } else if (name == "invert") {
        spec.kind = FilterKind::invert; require_only(values, {}, name);
    } else if (name == "resize-nearest" || name == "resize-bilinear") {
        spec.kind = name == "resize-nearest" ? FilterKind::resize_nearest : FilterKind::resize_bilinear;
        require_only(values, {"width", "height"}, name);
        const auto width = values.find("width"), height = values.find("height");
        if (width == values.end() || height == values.end())
            throw std::invalid_argument(name + " requires width and height");
        spec.width = parse_size(width->second, "width");
        spec.height = parse_size(height->second, "height");
    } else {
        throw std::invalid_argument("unknown filter: " + name);
    }
    return spec;
}

Image apply_filter(const Image& image, const FilterSpec& filter) {
    switch (filter.kind) {
        case FilterKind::blur: return gaussian_blur(image, filter.radius, filter.sigma);
        case FilterKind::sobel: return sobel_edges(image);
        case FilterKind::color: return color_adjust(image, filter.brightness, filter.contrast, filter.saturation);
        case FilterKind::grayscale: return grayscale(image);
        case FilterKind::sharpen: return sharpen(image, filter.amount);
        case FilterKind::invert: return invert(image);
        case FilterKind::resize_nearest: return resize_nearest(image, filter.width, filter.height);
        case FilterKind::resize_bilinear: return resize_bilinear(image, filter.width, filter.height);
    }
    throw std::invalid_argument("invalid filter kind");
}

Image apply_pipeline(Image image, const std::vector<FilterSpec>& filters) {
    for (const auto& filter : filters) image = apply_filter(image, filter);
    return image;
}

} // namespace prismforge

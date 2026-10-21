#include "prismforge/codec.hpp"
#include "prismforge/pipeline.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
struct Options {
    std::string input, output, input_dir, output_dir, format, effect;
    std::vector<std::string> filters;
    std::string radius, sigma, brightness, contrast, saturation, amount, width, height;
    int jpeg_quality{90};
    bool overwrite{false}, help{false}, version{false};
};

void usage() {
    std::cerr << "Prism-Forge " << PRISMFORGE_VERSION << " — CPU image processing\n"
              << "Single image: prismforge --input in.png --output out.png [--filter 'blur:radius=3,sigma=1.5']...\n"
              << "Legacy:       prismforge --input in.ppm --output out.ppm --effect blur --radius 3 --sigma 1.5\n"
              << "Batch:        prismforge --input-dir samples --output-dir processed [--format png] [--filter grayscale]\n"
              << "Filters: blur, sobel, color, grayscale, sharpen, invert, resize-nearest, resize-bilinear\n"
              << "Options: --quality 1..100 (JPEG), --overwrite, --help, --version\n";
}

Options parse_options(int argc, char** argv) {
    Options options;
    const auto value_for = [&](int& index, const std::string& key) -> std::string {
        if (index + 1 >= argc) throw std::invalid_argument("missing value for " + key);
        return argv[++index];
    };
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help" || key == "-h") options.help = true;
        else if (key == "--version") options.version = true;
        else if (key == "--overwrite") options.overwrite = true;
        else if (key == "--input") options.input = value_for(i, key);
        else if (key == "--output") options.output = value_for(i, key);
        else if (key == "--input-dir") options.input_dir = value_for(i, key);
        else if (key == "--output-dir") options.output_dir = value_for(i, key);
        else if (key == "--format") options.format = value_for(i, key);
        else if (key == "--effect") options.effect = value_for(i, key);
        else if (key == "--filter") options.filters.push_back(value_for(i, key));
        else if (key == "--radius") options.radius = value_for(i, key);
        else if (key == "--sigma") options.sigma = value_for(i, key);
        else if (key == "--brightness") options.brightness = value_for(i, key);
        else if (key == "--contrast") options.contrast = value_for(i, key);
        else if (key == "--saturation") options.saturation = value_for(i, key);
        else if (key == "--amount") options.amount = value_for(i, key);
        else if (key == "--width") options.width = value_for(i, key);
        else if (key == "--height") options.height = value_for(i, key);
        else if (key == "--quality") {
            const auto value = value_for(i, key);
            std::size_t used = 0;
            options.jpeg_quality = std::stoi(value, &used);
            if (used != value.size() || options.jpeg_quality < 1 || options.jpeg_quality > 100)
                throw std::invalid_argument("JPEG quality must be an integer in 1..100");
        } else throw std::invalid_argument("unknown option: " + key);
    }
    return options;
}

std::vector<prismforge::FilterSpec> make_pipeline(const Options& options) {
    if (!options.effect.empty() && !options.filters.empty())
        throw std::invalid_argument("use either legacy --effect or repeated --filter options, not both");
    if (!options.effect.empty()) {
        std::string expression = options.effect;
        std::vector<std::string> parameters;
        const auto append = [&](const char* name, const std::string& value) {
            if (!value.empty()) parameters.push_back(std::string(name) + "=" + value);
        };
        append("radius", options.radius); append("sigma", options.sigma);
        append("brightness", options.brightness); append("contrast", options.contrast);
        append("saturation", options.saturation); append("amount", options.amount);
        append("width", options.width); append("height", options.height);
        if (!parameters.empty()) {
            expression += ':';
            for (std::size_t i = 0; i < parameters.size(); ++i) {
                if (i != 0) expression += ',';
                expression += parameters[i];
            }
        }
        return {prismforge::parse_filter_spec(expression)};
    }
    if (!options.radius.empty() || !options.sigma.empty() || !options.brightness.empty() ||
        !options.contrast.empty() || !options.saturation.empty() || !options.amount.empty() ||
        !options.width.empty() || !options.height.empty())
        throw std::invalid_argument("filter parameters require --effect or --filter");
    std::vector<prismforge::FilterSpec> filters;
    filters.reserve(options.filters.size());
    for (const auto& filter : options.filters) filters.push_back(prismforge::parse_filter_spec(filter));
    return filters;
}

std::string normalized_format(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (value == "jpeg") return "jpg";
    if (value == "ppm" || value == "png" || value == "jpg") return value;
    throw std::invalid_argument("--format must be ppm, png, or jpeg");
}

bool path_contains(const fs::path& parent, const fs::path& child) {
    auto parent_text = parent.lexically_normal().generic_string();
    auto child_text = child.lexically_normal().generic_string();
#ifdef _WIN32
    const auto lower = [](std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    };
    parent_text = lower(std::move(parent_text));
    child_text = lower(std::move(child_text));
#endif
    if (parent_text == child_text) return true;
    if (!parent_text.empty() && parent_text.back() != '/') parent_text.push_back('/');
    return child_text.compare(0, parent_text.size(), parent_text) == 0;
}

int run_batch(const Options& options, const std::vector<prismforge::FilterSpec>& filters) {
    const fs::path input_dir(options.input_dir), output_dir(options.output_dir);
    std::error_code error;
    if (!fs::is_directory(input_dir, error) || error) throw std::runtime_error("input directory does not exist or is not a directory");
    const auto input_real = fs::weakly_canonical(input_dir, error);
    if (error) throw std::runtime_error("cannot resolve input directory: " + error.message());
    const auto output_real = fs::weakly_canonical(output_dir, error);
    if (error) throw std::runtime_error("cannot resolve output directory: " + error.message());
    if (path_contains(input_real, output_real) || path_contains(output_real, input_real))
        throw std::runtime_error("input and output directories must not overlap");
    fs::create_directories(output_dir);

    const auto format = options.format.empty() ? std::string{} : normalized_format(options.format);
    std::vector<fs::path> inputs;
    for (const auto& entry : fs::directory_iterator(input_dir))
        if (entry.is_regular_file()) inputs.push_back(entry.path());
    std::sort(inputs.begin(), inputs.end());

    // Preflight naming collisions before processing anything. With --format png,
    // photo.jpg and photo.png otherwise target the same photo.png destination.
    // Even --overwrite must not silently discard a different input in this batch.
    std::map<std::string, fs::path> output_owners;
    for (const auto& input_path : inputs) {
        if (!prismforge::is_supported_image_extension(input_path)) continue;
        fs::path output_path = output_dir / input_path.filename();
        if (!format.empty()) output_path.replace_extension("." + format);
        auto output_key = output_path.lexically_normal().generic_string();
#ifdef _WIN32
        std::transform(output_key.begin(), output_key.end(), output_key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
#endif
        const auto insertion = output_owners.emplace(std::move(output_key), input_path);
        if (!insertion.second) {
            throw std::runtime_error("batch output name collision: " +
                                     insertion.first->second.filename().string() + " and " +
                                     input_path.filename().string() + " both target " +
                                     output_path.filename().string() +
                                     "; rename an input or process separately");
        }
    }

    std::size_t processed = 0, skipped = 0, failed = 0;
    for (const auto& input_path : inputs) {
        if (!prismforge::is_supported_image_extension(input_path)) { ++skipped; continue; }
        fs::path output_path = output_dir / input_path.filename();
        if (!format.empty()) output_path.replace_extension("." + format);
        bool output_preexisted = fs::exists(output_path);
        try {
            if (!options.overwrite && output_preexisted)
                throw std::runtime_error("output already exists (use --overwrite): " + output_path.string());
            const auto image = prismforge::load_image(input_path);
            const auto result = prismforge::apply_pipeline(image, filters);
            prismforge::save_image(result, output_path, options.jpeg_quality);
            ++processed;
        } catch (const std::exception& exception) {
            if (!output_preexisted) { std::error_code ignored; fs::remove(output_path, ignored); }
            std::cerr << input_path.filename().string() << ": " << exception.what() << '\n';
            ++failed;
        }
    }
    std::cout << "Batch complete: " << processed << " processed, " << skipped
              << " unsupported skipped, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
}

int main(int argc, char** argv) {
    try {
        const auto options = parse_options(argc, argv);
        if (options.help) { usage(); return 0; }
        if (options.version) { std::cout << "Prism-Forge " << PRISMFORGE_VERSION << '\n'; return 0; }
        const auto filters = make_pipeline(options);
        const bool batch = !options.input_dir.empty() || !options.output_dir.empty();
        if (batch) {
            if (options.input_dir.empty() || options.output_dir.empty() || !options.input.empty() || !options.output.empty())
                throw std::invalid_argument("batch mode requires --input-dir and --output-dir only");
            return run_batch(options, filters);
        }
        if (options.input.empty() || options.output.empty())
            throw std::invalid_argument("single-image mode requires --input and --output");
        if (!options.format.empty()) throw std::invalid_argument("--format is available only in batch mode");
        const fs::path output_path(options.output);
        if (!options.overwrite && fs::exists(output_path))
            throw std::runtime_error("output already exists (use --overwrite): " + output_path.string());
        const auto image = prismforge::load_image(options.input);
        const auto result = prismforge::apply_pipeline(image, filters);
        prismforge::save_image(result, output_path, options.jpeg_quality);
        std::cout << "Processed " << image.width() << 'x' << image.height() << " image with "
                  << filters.size() << " filter(s) -> " << output_path.string() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "prismforge: " << error.what() << '\n'; usage(); return 2;
    }
}

#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <gdiplus.h>
#include <shellapi.h>

#include "prismforge/codec.hpp"
#include "prismforge/pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cwchar>
#include <cwctype>
#include <filesystem>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

namespace {
using prismforge::FilterSpec;
using prismforge::Image;
using prismforge::Pixel;
using Gdiplus::Bitmap;

constexpr wchar_t kWindowClass[] = L"PrismForgeNativeWindow";
constexpr UINT kTaskFinished = WM_APP + 7;
constexpr int kOpen = 1001;
constexpr int kSave = 1002;
constexpr int kFilter = 1003;
constexpr int kParameters = 1004;
constexpr int kApply = 1005;
constexpr int kHistory = 1006;
constexpr int kUp = 1007;
constexpr int kDown = 1008;
constexpr int kRemove = 1009;
constexpr int kUndo = 1010;
constexpr int kReset = 1011;
constexpr int kQuality = 1012;
constexpr int kHelp = 1013;
constexpr int kInfo = 1014;
constexpr int kStatus = 1015;

const wchar_t* const kFilterNames[]{L"Gaussian blur", L"Sobel edges", L"Color adjustment", L"Grayscale",
                                    L"Sharpen", L"Invert", L"Resize (nearest)", L"Resize (bilinear)"};
const char* const kFilterKeys[]{"blur", "sobel", "color", "grayscale", "sharpen", "invert",
                                "resize-nearest", "resize-bilinear"};

enum class JobKind { open, process, preview, save };

struct JobResult {
    JobKind kind{JobKind::preview};
    std::shared_ptr<Image> image;
    std::unique_ptr<Bitmap> preview;
    std::filesystem::path path;
    std::string error;
};

struct AppState {
    HWND window{};
    HWND openButton{};
    HWND saveButton{};
    HWND filterCombo{};
    HWND parameterEdit{};
    HWND helpLabel{};
    HWND infoLabel{};
    HWND historyList{};
    HWND qualityEdit{};
    HWND statusBar{};
    std::shared_ptr<Image> original;
    std::shared_ptr<Image> current;
    std::unique_ptr<Bitmap> preview;
    std::vector<FilterSpec> filters;
    std::filesystem::path sourcePath;
    std::thread worker;
    bool busy{false};
    bool closeWhenIdle{false};
};

AppState g_app;

std::wstring to_wide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return L"Image processing failed.";
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count);
    return result;
}

std::string to_utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) throw std::invalid_argument("Filter parameters contain unsupported text.");
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count, nullptr, nullptr);
    return result;
}

std::wstring control_text(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring value(static_cast<std::size_t>(std::max(length, 0)) + 1U, L'\0');
    const int read = GetWindowTextW(control, value.data(), static_cast<int>(value.size()));
    value.resize(static_cast<std::size_t>(std::max(read, 0)));
    return value;
}

void set_status(const std::wstring& message) {
    if (g_app.statusBar != nullptr) SendMessageW(g_app.statusBar, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(message.c_str()));
}

void show_error(const std::string& message) {
    MessageBoxW(g_app.window, to_wide(message).c_str(), L"Prism-Forge", MB_OK | MB_ICONERROR);
}

void set_font(HWND control) {
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
}

HWND add_control(HWND parent, const wchar_t* class_name, const wchar_t* text, DWORD style,
                 int id, int x, int y, int width, int height) {
    HWND control = CreateWindowExW(0, class_name, text, WS_CHILD | WS_VISIBLE | style,
                                  x, y, width, height, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                  GetModuleHandleW(nullptr), nullptr);
    if (control != nullptr) set_font(control);
    return control;
}

std::unique_ptr<Bitmap> make_preview(const Image& image) {
    constexpr std::size_t max_width = 1500;
    constexpr std::size_t max_height = 1100;
    const double scale = std::min({1.0, static_cast<double>(max_width) / static_cast<double>(image.width()),
                                   static_cast<double>(max_height) / static_cast<double>(image.height())});
    const auto width = std::max<std::size_t>(1, static_cast<std::size_t>(static_cast<double>(image.width()) * scale));
    const auto height = std::max<std::size_t>(1, static_cast<std::size_t>(static_cast<double>(image.height()) * scale));
    auto bitmap = std::make_unique<Bitmap>(static_cast<INT>(width), static_cast<INT>(height), PixelFormat32bppARGB);
    if (bitmap->GetLastStatus() != Gdiplus::Ok) throw std::runtime_error("Could not allocate the image preview.");

    Gdiplus::Rect rectangle(0, 0, static_cast<INT>(width), static_cast<INT>(height));
    Gdiplus::BitmapData data{};
    if (bitmap->LockBits(&rectangle, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &data) != Gdiplus::Ok)
        throw std::runtime_error("Could not prepare the image preview.");
    for (std::size_t y = 0; y < height; ++y) {
        auto* row = static_cast<BYTE*>(data.Scan0) + static_cast<std::ptrdiff_t>(y) * data.Stride;
        const std::size_t source_y = std::min(image.height() - 1, y * image.height() / height);
        for (std::size_t x = 0; x < width; ++x) {
            const std::size_t source_x = std::min(image.width() - 1, x * image.width() / width);
            const Pixel pixel = image.at(source_x, source_y);
            auto* destination = row + x * 4U;
            destination[0] = pixel.b;
            destination[1] = pixel.g;
            destination[2] = pixel.r;
            destination[3] = pixel.a;
        }
    }
    if (bitmap->UnlockBits(&data) != Gdiplus::Ok) throw std::runtime_error("Could not finish the image preview.");
    return bitmap;
}

Image flattened_on_white(const Image& image) {
    Image output(image.width(), image.height());
    for (std::size_t y = 0; y < image.height(); ++y) for (std::size_t x = 0; x < image.width(); ++x) {
        const Pixel source = image.at(x, y);
        const auto flatten = [alpha = static_cast<unsigned>(source.a)](std::uint8_t channel) {
            return static_cast<std::uint8_t>((static_cast<unsigned>(channel) * alpha + 255U * (255U - alpha) + 127U) / 255U);
        };
        output.at(x, y) = {flatten(source.r), flatten(source.g), flatten(source.b), 255};
    }
    return output;
}

void update_info() {
    if (g_app.infoLabel == nullptr || g_app.current == nullptr) return;
    const std::wstring name = g_app.sourcePath.empty() ? L"Untitled image" : g_app.sourcePath.filename().wstring();
    const std::wstring text = name + L"  |  " + std::to_wstring(g_app.current->width()) + L" \u00D7 " +
                              std::to_wstring(g_app.current->height()) + L" px";
    SetWindowTextW(g_app.infoLabel, text.c_str());
}

std::wstring filter_description(const FilterSpec& filter) {
    switch (filter.kind) {
    case prismforge::FilterKind::blur:
        return L"Gaussian blur  (radius " + std::to_wstring(filter.radius) + L", sigma " + std::to_wstring(filter.sigma) + L")";
    case prismforge::FilterKind::sobel: return L"Sobel edges";
    case prismforge::FilterKind::color:
        return L"Color adjustment  (brightness " + std::to_wstring(filter.brightness) + L", contrast " +
               std::to_wstring(filter.contrast) + L", saturation " + std::to_wstring(filter.saturation) + L")";
    case prismforge::FilterKind::grayscale: return L"Grayscale";
    case prismforge::FilterKind::sharpen: return L"Sharpen  (amount " + std::to_wstring(filter.amount) + L")";
    case prismforge::FilterKind::invert: return L"Invert";
    case prismforge::FilterKind::resize_nearest:
        return L"Resize nearest  (" + std::to_wstring(filter.width) + L" \u00D7 " + std::to_wstring(filter.height) + L")";
    case prismforge::FilterKind::resize_bilinear:
        return L"Resize bilinear  (" + std::to_wstring(filter.width) + L" \u00D7 " + std::to_wstring(filter.height) + L")";
    }
    return L"Filter";
}

void rebuild_history() {
    SendMessageW(g_app.historyList, LB_RESETCONTENT, 0, 0);
    for (std::size_t i = 0; i < g_app.filters.size(); ++i) {
        const std::wstring row = std::to_wstring(i + 1) + L". " + filter_description(g_app.filters[i]);
        SendMessageW(g_app.historyList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
    if (!g_app.filters.empty()) SendMessageW(g_app.historyList, LB_SETCURSEL, g_app.filters.size() - 1, 0);
}

void update_filter_help() {
    const LRESULT selection = SendMessageW(g_app.filterCombo, CB_GETCURSEL, 0, 0);
    if (selection < 0 || selection >= static_cast<LRESULT>(std::size(kFilterKeys))) return;
    std::wstring help;
    switch (selection) {
    case 0: help = L"radius=0..128,sigma=1.5. Example: radius=3,sigma=1.5"; break;
    case 1: help = L"Finds edges and converts the result to grayscale. No parameters."; break;
    case 2: help = L"brightness=0,contrast=1,saturation=1. Example: brightness=10,contrast=1.1,saturation=1.2"; break;
    case 3: help = L"Converts to Rec. 601 grayscale. No parameters."; break;
    case 4: help = L"amount=0..10. Example: amount=0.8"; break;
    case 5: help = L"Inverts the image colors. No parameters."; break;
    case 6: case 7: help = L"Set both output dimensions. Example: width=800,height=600"; break;
    default: return;
    }
    SetWindowTextW(g_app.helpLabel, help.c_str());
}

void set_busy(bool busy) {
    g_app.busy = busy;
    EnableWindow(g_app.openButton, !busy);
    EnableWindow(g_app.saveButton, !busy && g_app.current != nullptr);
    EnableWindow(g_app.filterCombo, !busy);
    EnableWindow(g_app.parameterEdit, !busy);
    EnableWindow(GetDlgItem(g_app.window, kApply), !busy && g_app.original != nullptr);
    EnableWindow(g_app.historyList, !busy);
    EnableWindow(GetDlgItem(g_app.window, kUp), !busy);
    EnableWindow(GetDlgItem(g_app.window, kDown), !busy);
    EnableWindow(GetDlgItem(g_app.window, kRemove), !busy);
    EnableWindow(GetDlgItem(g_app.window, kUndo), !busy);
    EnableWindow(GetDlgItem(g_app.window, kReset), !busy);
    EnableWindow(g_app.qualityEdit, !busy);
    if (busy) SetCursor(LoadCursorW(nullptr, IDC_WAIT));
    else SetCursor(LoadCursorW(nullptr, IDC_ARROW));
}

void worker_main(HWND window, JobKind kind, std::filesystem::path path,
                 std::shared_ptr<Image> image, std::vector<FilterSpec> filters, int quality, bool flatten_alpha) {
    auto result = std::make_unique<JobResult>();
    result->kind = kind;
    result->path = std::move(path);
    try {
        if (kind == JobKind::open) {
            result->image = std::make_shared<Image>(prismforge::load_image(result->path));
            result->preview = make_preview(*result->image);
        } else if (kind == JobKind::process) {
            result->image = std::make_shared<Image>(prismforge::apply_pipeline(*image, filters));
            result->preview = make_preview(*result->image);
        } else if (kind == JobKind::preview) {
            result->image = std::move(image);
            result->preview = make_preview(*result->image);
        } else {
            if (flatten_alpha) {
                const Image flattened = flattened_on_white(*image);
                prismforge::save_image(flattened, result->path, quality);
            } else {
                prismforge::save_image(*image, result->path, quality);
            }
        }
    } catch (const std::exception& error) {
        result->error = error.what();
    } catch (...) {
        result->error = "An unexpected error occurred while processing the image.";
    }
    if (PostMessageW(window, kTaskFinished, 0, reinterpret_cast<LPARAM>(result.get())) != 0) result.release();
}

void start_job(JobKind kind, std::filesystem::path path = {}, std::shared_ptr<Image> image = {},
               std::vector<FilterSpec> filters = {}, int quality = 90, bool flatten_alpha = false) {
    if (g_app.busy) return;
    set_busy(true);
    switch (kind) {
    case JobKind::open: set_status(L"Opening image\u2026"); break;
    case JobKind::process: set_status(L"Applying filter steps\u2026"); break;
    case JobKind::preview: set_status(L"Updating preview\u2026"); break;
    case JobKind::save: set_status(L"Saving image\u2026"); break;
    }
    try {
        g_app.worker = std::thread(worker_main, g_app.window, kind, std::move(path), std::move(image),
                                   std::move(filters), quality, flatten_alpha);
    } catch (const std::exception& error) {
        set_busy(false);
        set_status(L"Ready");
        show_error(std::string("Could not start background image processing: ") + error.what());
    }
}

void process_filters() {
    if (g_app.original == nullptr) return;
    if (g_app.filters.empty()) {
        start_job(JobKind::preview, {}, g_app.original);
    } else {
        start_job(JobKind::process, {}, g_app.original, g_app.filters);
    }
}

bool choose_open_path(std::filesystem::path& path) {
    wchar_t filename[32768]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_app.window;
    dialog.lpstrFilter = L"Supported images (*.png;*.jpg;*.jpeg;*.ppm)\0*.png;*.jpg;*.jpeg;*.ppm\0PNG images (*.png)\0*.png\0JPEG images (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0PPM images (*.ppm)\0*.ppm\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = filename;
    dialog.nMaxFile = static_cast<DWORD>(std::size(filename));
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    dialog.lpstrTitle = L"Open an image";
    if (GetOpenFileNameW(&dialog) == FALSE) return false;
    path = std::filesystem::path(filename);
    return true;
}

bool choose_save_path(std::filesystem::path& path) {
    wchar_t filename[32768]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = g_app.window;
    dialog.lpstrFilter = L"PNG image (*.png)\0*.png\0JPEG image (*.jpg)\0*.jpg\0PPM image (*.ppm)\0*.ppm\0\0";
    dialog.lpstrFile = filename;
    dialog.nMaxFile = static_cast<DWORD>(std::size(filename));
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    dialog.lpstrTitle = L"Save processed image";
    if (GetSaveFileNameW(&dialog) == FALSE) return false;
    path = std::filesystem::path(filename);
    if (path.extension().empty()) {
        switch (dialog.nFilterIndex) {
        case 2: path += L".jpg"; break;
        case 3: path += L".ppm"; break;
        default: path += L".png"; break;
        }
    }
    return true;
}

bool has_transparency(const Image& image) {
    return std::any_of(image.pixels().begin(), image.pixels().end(), [](Pixel pixel) { return pixel.a != 255; });
}

int parse_quality() {
    const std::wstring text = control_text(g_app.qualityEdit);
    if (text.empty()) return 90;
    wchar_t* end = nullptr;
    const long value = std::wcstol(text.c_str(), &end, 10);
    if (end == text.c_str() || *end != L'\0' || value < 1 || value > 100)
        throw std::invalid_argument("JPEG quality must be a whole number from 1 to 100.");
    return static_cast<int>(value);
}

void layout_controls(HWND window) {
    RECT client{};
    GetClientRect(window, &client);
    SendMessageW(g_app.statusBar, WM_SIZE, 0, 0);
    RECT status_rect{};
    GetWindowRect(g_app.statusBar, &status_rect);
    const int status_height = status_rect.bottom - status_rect.top;
    const int height = client.bottom - status_height;
    constexpr int panel_width = 390;
    const int left = 20;
    const int inner_width = panel_width - left * 2;
    MoveWindow(g_app.openButton, left, 16, 150, 34, TRUE);
    MoveWindow(g_app.saveButton, left + 164, 16, inner_width - 164, 34, TRUE);
    MoveWindow(g_app.infoLabel, left, 58, inner_width, 32, TRUE);
    MoveWindow(GetDlgItem(window, 1201), left, 98, inner_width, 22, TRUE);
    MoveWindow(g_app.filterCombo, left, 120, inner_width, 250, TRUE);
    MoveWindow(GetDlgItem(window, 1202), left, 158, inner_width, 22, TRUE);
    MoveWindow(g_app.parameterEdit, left, 180, inner_width, 27, TRUE);
    MoveWindow(g_app.helpLabel, left, 211, inner_width, 48, TRUE);
    MoveWindow(GetDlgItem(window, kApply), left, 262, inner_width, 34, TRUE);
    MoveWindow(GetDlgItem(window, 1203), left, 307, inner_width, 22, TRUE);
    const int list_height = std::max(120, height - 477);
    MoveWindow(g_app.historyList, left, 330, inner_width, list_height, TRUE);
    const int action_y = 338 + list_height;
    MoveWindow(GetDlgItem(window, kUp), left, action_y, 74, 30, TRUE);
    MoveWindow(GetDlgItem(window, kDown), left + 80, action_y, 74, 30, TRUE);
    MoveWindow(GetDlgItem(window, kRemove), left + 160, action_y, inner_width - 160, 30, TRUE);
    MoveWindow(GetDlgItem(window, kUndo), left, action_y + 40, 148, 32, TRUE);
    MoveWindow(GetDlgItem(window, kReset), left + 156, action_y + 40, inner_width - 156, 32, TRUE);
    MoveWindow(GetDlgItem(window, 1204), left, action_y + 84, 220, 22, TRUE);
    MoveWindow(g_app.qualityEdit, left + 224, action_y + 80, 92, 26, TRUE);
    InvalidateRect(window, nullptr, TRUE);
}

void draw_preview(HWND window, HDC hdc) {
    RECT client{};
    GetClientRect(window, &client);
    RECT status_rect{};
    GetWindowRect(g_app.statusBar, &status_rect);
    const int height = client.bottom - (status_rect.bottom - status_rect.top);
    RECT area{390, 0, client.right, height};
    HBRUSH background = CreateSolidBrush(RGB(244, 246, 249));
    FillRect(hdc, &area, background);
    DeleteObject(background);
    if (g_app.preview == nullptr) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(100, 110, 125));
        DrawTextW(hdc, L"Open an image to begin. Choose a filter, add it to the list, then apply your edits.", -1,
                  &area, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
        return;
    }
    const auto image_width = static_cast<double>(g_app.preview->GetWidth());
    const auto image_height = static_cast<double>(g_app.preview->GetHeight());
    const double scale = std::min(static_cast<double>(std::max(1L, area.right - area.left - 36L)) / image_width,
                                  static_cast<double>(std::max(1L, area.bottom - area.top - 36L)) / image_height);
    const int draw_width = static_cast<int>(image_width * scale);
    const int draw_height = static_cast<int>(image_height * scale);
    const int x = area.left + ((area.right - area.left) - draw_width) / 2;
    const int y = area.top + ((area.bottom - area.top) - draw_height) / 2;
    Gdiplus::Graphics graphics(hdc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.DrawImage(g_app.preview.get(), x, y, draw_width, draw_height);
}

void finish_job(JobResult& result) {
    set_busy(false);
    if (!result.error.empty()) {
        set_status(L"Ready \u2014 the operation could not be completed.");
        show_error(result.error);
        return;
    }
    if (result.kind == JobKind::open) {
        g_app.original = result.image;
        g_app.current = result.image;
        g_app.preview = std::move(result.preview);
        g_app.filters.clear();
        g_app.sourcePath = result.path;
        rebuild_history();
        update_info();
        EnableWindow(g_app.saveButton, TRUE);
        EnableWindow(GetDlgItem(g_app.window, kApply), TRUE);
        set_status(L"Image opened. Add filters to build an edit list.");
    } else if (result.kind == JobKind::process || result.kind == JobKind::preview) {
        g_app.current = result.image;
        g_app.preview = std::move(result.preview);
        update_info();
        set_status(result.kind == JobKind::preview && g_app.filters.empty() ? L"Original image restored." : L"Preview updated.");
    } else {
        set_status(L"Image saved successfully.");
    }
    InvalidateRect(g_app.window, nullptr, TRUE);
}

void apply_selected_filter() {
    const LRESULT selection = SendMessageW(g_app.filterCombo, CB_GETCURSEL, 0, 0);
    if (selection < 0 || selection >= static_cast<LRESULT>(std::size(kFilterKeys))) return;
    try {
        std::string expression = kFilterKeys[selection];
        std::wstring parameters = control_text(g_app.parameterEdit);
        const auto first = parameters.find_first_not_of(L" \t\r\n");
        if (first != std::wstring::npos) {
            const auto last = parameters.find_last_not_of(L" \t\r\n");
            expression += ':' + to_utf8(parameters.substr(first, last - first + 1));
        }
        const FilterSpec filter = prismforge::parse_filter_spec(expression);
        if (g_app.original == nullptr) {
            MessageBoxW(g_app.window, L"Open an image before adding a filter.", L"Prism-Forge", MB_OK | MB_ICONINFORMATION);
            return;
        }
        g_app.filters.push_back(filter);
        rebuild_history();
        process_filters();
    } catch (const std::exception& error) {
        show_error(error.what());
    }
}

void save_current() {
    if (g_app.current == nullptr) return;
    std::filesystem::path path;
    if (!choose_save_path(path)) return;
    try {
        const std::wstring extension = path.extension().wstring();
        std::wstring lowered = extension;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        const std::shared_ptr<Image> image = g_app.current;
        bool flatten_alpha = false;
        if ((lowered == L".jpg" || lowered == L".jpeg" || lowered == L".ppm") && has_transparency(*image)) {
            const int answer = MessageBoxW(g_app.window,
                L"This image has transparent pixels. JPEG and PPM cannot store transparency.\n\n"
                L"Continue and composite transparent pixels onto white?",
                L"Transparency will be flattened", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
            if (answer != IDYES) return;
            flatten_alpha = true;
        }
        const int quality = (lowered == L".jpg" || lowered == L".jpeg") ? parse_quality() : 90;
        start_job(JobKind::save, std::move(path), image, {}, quality, flatten_alpha);
    } catch (const std::exception& error) {
        show_error(error.what());
    }
}

void open_image() {
    if (!g_app.filters.empty()) {
        const int answer = MessageBoxW(g_app.window,
            L"Opening another image will discard the current filter list. Continue?",
            L"Discard current edits?", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (answer != IDYES) return;
    }
    std::filesystem::path path;
    if (!choose_open_path(path)) return;
    start_job(JobKind::open, std::move(path));
}

void reorder_filter(int direction) {
    const LRESULT selected = SendMessageW(g_app.historyList, LB_GETCURSEL, 0, 0);
    if (selected == LB_ERR) return;
    const auto index = static_cast<std::size_t>(selected);
    const auto destination = static_cast<std::ptrdiff_t>(index) + direction;
    if (destination < 0 || destination >= static_cast<std::ptrdiff_t>(g_app.filters.size())) return;
    std::swap(g_app.filters[index], g_app.filters[static_cast<std::size_t>(destination)]);
    rebuild_history();
    SendMessageW(g_app.historyList, LB_SETCURSEL, static_cast<WPARAM>(destination), 0);
    process_filters();
}

void remove_selected_filter() {
    LRESULT selected = SendMessageW(g_app.historyList, LB_GETCURSEL, 0, 0);
    if (selected == LB_ERR && !g_app.filters.empty()) selected = static_cast<LRESULT>(g_app.filters.size() - 1);
    if (selected == LB_ERR) return;
    g_app.filters.erase(g_app.filters.begin() + selected);
    rebuild_history();
    process_filters();
}

void create_controls(HWND window) {
    g_app.window = window;
    g_app.openButton = add_control(window, L"BUTTON", L"Open Image\u2026", BS_PUSHBUTTON | WS_TABSTOP, kOpen, 0, 0, 0, 0);
    g_app.saveButton = add_control(window, L"BUTTON", L"Save As\u2026", BS_PUSHBUTTON | WS_TABSTOP, kSave, 0, 0, 0, 0);
    g_app.infoLabel = add_control(window, L"STATIC", L"No image open", SS_LEFT | SS_NOPREFIX, kInfo, 0, 0, 0, 0);
    add_control(window, L"STATIC", L"Choose a filter", SS_LEFT, 1201, 0, 0, 0, 0);
    g_app.filterCombo = add_control(window, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, kFilter, 0, 0, 0, 0);
    for (const auto* name : kFilterNames) SendMessageW(g_app.filterCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
    SendMessageW(g_app.filterCombo, CB_SETCURSEL, 0, 0);
    add_control(window, L"STATIC", L"Parameters (name=value, comma separated)", SS_LEFT, 1202, 0, 0, 0, 0);
    g_app.parameterEdit = add_control(window, L"EDIT", L"radius=3,sigma=1.5", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP,
                                      kParameters, 0, 0, 0, 0);
    g_app.helpLabel = add_control(window, L"STATIC", L"", SS_LEFT | SS_NOPREFIX, kHelp, 0, 0, 0, 0);
    add_control(window, L"BUTTON", L"Apply Filter", BS_DEFPUSHBUTTON | WS_TABSTOP, kApply, 0, 0, 0, 0);
    add_control(window, L"STATIC", L"Filter steps (top to bottom)", SS_LEFT, 1203, 0, 0, 0, 0);
    g_app.historyList = add_control(window, L"LISTBOX", L"", LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_BORDER | WS_VSCROLL | WS_TABSTOP,
                                    kHistory, 0, 0, 0, 0);
    add_control(window, L"BUTTON", L"Move up", BS_PUSHBUTTON | WS_TABSTOP, kUp, 0, 0, 0, 0);
    add_control(window, L"BUTTON", L"Move down", BS_PUSHBUTTON | WS_TABSTOP, kDown, 0, 0, 0, 0);
    add_control(window, L"BUTTON", L"Remove", BS_PUSHBUTTON | WS_TABSTOP, kRemove, 0, 0, 0, 0);
    add_control(window, L"BUTTON", L"Undo last", BS_PUSHBUTTON | WS_TABSTOP, kUndo, 0, 0, 0, 0);
    add_control(window, L"BUTTON", L"Reset", BS_PUSHBUTTON | WS_TABSTOP, kReset, 0, 0, 0, 0);
    add_control(window, L"STATIC", L"JPEG quality (1\u2013100; default 90)", SS_LEFT, 1204, 0, 0, 0, 0);
    g_app.qualityEdit = add_control(window, L"EDIT", L"90", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, kQuality, 0, 0, 0, 0);
    g_app.statusBar = CreateWindowExW(0, STATUSCLASSNAMEW, L"Ready", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                      0, 0, 0, 0, window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kStatus)),
                                      GetModuleHandleW(nullptr), nullptr);
    layout_controls(window);
    update_filter_help();
    EnableWindow(g_app.saveButton, FALSE);
    EnableWindow(GetDlgItem(window, kApply), FALSE);
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE:
        create_controls(window);
        return 0;
    case WM_SIZE:
        layout_controls(window);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lparam);
        info->ptMinTrackSize.x = 900;
        info->ptMinTrackSize.y = 650;
        return 0;
    }
    case WM_COMMAND:
        if (HIWORD(wparam) == CBN_SELCHANGE && LOWORD(wparam) == kFilter) {
            const LRESULT selection = SendMessageW(g_app.filterCombo, CB_GETCURSEL, 0, 0);
            static const wchar_t* const defaults[]{L"radius=3,sigma=1.5", L"", L"brightness=0,contrast=1,saturation=1", L"",
                                                   L"amount=1", L"", L"width=800,height=600", L"width=800,height=600"};
            if (selection >= 0 && selection < static_cast<LRESULT>(std::size(defaults)))
                SetWindowTextW(g_app.parameterEdit, defaults[selection]);
            update_filter_help();
            return 0;
        }
        if (HIWORD(wparam) == BN_CLICKED) {
            switch (LOWORD(wparam)) {
            case kOpen: open_image(); return 0;
            case kSave: save_current(); return 0;
            case kApply: apply_selected_filter(); return 0;
            case kUp: reorder_filter(-1); return 0;
            case kDown: reorder_filter(1); return 0;
            case kRemove: remove_selected_filter(); return 0;
            case kUndo:
                if (!g_app.filters.empty()) { g_app.filters.pop_back(); rebuild_history(); process_filters(); }
                return 0;
            case kReset:
                g_app.filters.clear();
                rebuild_history();
                process_filters();
                return 0;
            default: break;
            }
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        draw_preview(window, dc);
        EndPaint(window, &paint);
        return 0;
    }
    case kTaskFinished: {
        std::unique_ptr<JobResult> result(reinterpret_cast<JobResult*>(lparam));
        if (g_app.worker.joinable()) g_app.worker.join();
        finish_job(*result);
        if (g_app.closeWhenIdle) DestroyWindow(window);
        return 0;
    }
    case WM_CLOSE:
        if (g_app.busy) {
            g_app.closeWhenIdle = true;
            set_status(L"Finishing the current operation before closing\u2026");
            return 0;
        }
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default: break;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

bool register_window_class(HINSTANCE instance) {
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    window_class.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    window_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    window_class.lpszClassName = kWindowClass;
    return RegisterClassExW(&window_class) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

int smoke_test(HINSTANCE instance, const std::filesystem::path& input, const std::filesystem::path& output) {
    INITCOMMONCONTROLSEX common_controls{sizeof(common_controls), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
    if (InitCommonControlsEx(&common_controls) == FALSE || !register_window_class(instance)) return 10;
    HWND hidden_window = CreateWindowExW(0, kWindowClass, L"Prism-Forge GUI smoke test", WS_OVERLAPPED,
                                         0, 0, 1000, 700, nullptr, nullptr, instance, nullptr);
    if (hidden_window == nullptr) return 11;
    try {
        auto image = std::make_shared<Image>(prismforge::load_image(input));
        const std::vector<FilterSpec> filters{
            prismforge::parse_filter_spec("grayscale"),
            prismforge::parse_filter_spec("sharpen:amount=0.8")};
        auto processed = std::make_shared<Image>(prismforge::apply_pipeline(*image, filters));
        auto preview = make_preview(*processed);
        if (preview == nullptr || preview->GetWidth() == 0 || preview->GetHeight() == 0) throw std::runtime_error("Preview smoke failed.");
        prismforge::save_image(*processed, output, 90);
        const Image reopened = prismforge::load_image(output);
        if (reopened.width() != processed->width() || reopened.height() != processed->height())
            throw std::runtime_error("Saved output dimensions did not match the processed image.");
        DestroyWindow(hidden_window);
        return 0;
    } catch (...) {
        DestroyWindow(hidden_window);
        return 12;
    }
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show_command) {
    SetProcessDPIAware();
    Gdiplus::GdiplusStartupInput gdiplus_input;
    ULONG_PTR gdiplus_token = 0;
    if (Gdiplus::GdiplusStartup(&gdiplus_token, &gdiplus_input, nullptr) != Gdiplus::Ok) {
        MessageBoxW(nullptr, L"Windows could not initialize image preview support.", L"Prism-Forge", MB_OK | MB_ICONERROR);
        return 1;
    }
    int exit_code = 0;
    int argument_count = 0;
    LPWSTR* arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    if (arguments == nullptr) {
        Gdiplus::GdiplusShutdown(gdiplus_token);
        return 2;
    }
    const bool smoke_mode = argument_count == 4 && std::wstring(arguments[1]) == L"--smoke-test";
    if (smoke_mode) {
        exit_code = smoke_test(instance, std::filesystem::path(arguments[2]), std::filesystem::path(arguments[3]));
    } else if (argument_count != 1) {
        MessageBoxW(nullptr, L"Prism-Forge GUI does not accept command-line arguments. Double-click the app to start.",
                    L"Prism-Forge", MB_OK | MB_ICONINFORMATION);
        exit_code = 2;
    } else {
        INITCOMMONCONTROLSEX common_controls{sizeof(common_controls), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
        if (InitCommonControlsEx(&common_controls) == FALSE || !register_window_class(instance)) {
            MessageBoxW(nullptr, L"Windows could not initialize the Prism-Forge window.", L"Prism-Forge", MB_OK | MB_ICONERROR);
            exit_code = 3;
        } else {
            HWND window = CreateWindowExW(0, kWindowClass, L"Prism-Forge \u2014 Image Editor",
                                          WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
                                          1180, 820, nullptr, nullptr, instance, nullptr);
            if (window == nullptr) {
                MessageBoxW(nullptr, L"Prism-Forge could not create its main window.", L"Prism-Forge", MB_OK | MB_ICONERROR);
                exit_code = 4;
            } else {
                ShowWindow(window, show_command);
                UpdateWindow(window);
                MSG message{};
                while (GetMessageW(&message, nullptr, 0, 0) > 0) {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                }
                exit_code = static_cast<int>(message.wParam);
            }
        }
    }
    LocalFree(arguments);
    Gdiplus::GdiplusShutdown(gdiplus_token);
    return exit_code;
}

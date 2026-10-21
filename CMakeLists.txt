cmake_minimum_required(VERSION 3.18)
cmake_policy(SET CMP0091 NEW)
project(PrismForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(PRISMFORGE_STRICT_WARNINGS "Enable strict compiler warnings" ON)
option(PRISMFORGE_BUILD_BENCHMARK "Build the lightweight benchmark" ON)
option(PRISMFORGE_ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

add_library(prismforge_core
    src/image.cpp
    src/filters.cpp
    src/ppm.cpp
    src/codec.cpp
    src/pipeline.cpp)
target_include_directories(prismforge_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(prismforge_core SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb)
target_compile_features(prismforge_core PUBLIC cxx_std_17)
target_compile_definitions(prismforge_core PUBLIC PRISMFORGE_VERSION="${PROJECT_VERSION}")
if(PRISMFORGE_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(prismforge_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(prismforge_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endif()
if(PRISMFORGE_ENABLE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "PRISMFORGE_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
    target_compile_options(prismforge_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(prismforge_core PUBLIC -fsanitize=address,undefined)
endif()

add_executable(prismforge src/main.cpp)
target_link_libraries(prismforge PRIVATE prismforge_core)
if(PRISMFORGE_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(prismforge PRIVATE /W4 /permissive-)
    else()
        target_compile_options(prismforge PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endif()

if(WIN32)
    add_executable(prismforge_gui WIN32 src/windows_gui.cpp)
    set_target_properties(prismforge_gui PROPERTIES OUTPUT_NAME "PrismForge-GUI")
    target_link_libraries(prismforge_gui PRIVATE prismforge_core comctl32 comdlg32 gdiplus shell32)
    if(PRISMFORGE_STRICT_WARNINGS)
        if(MSVC)
            target_compile_options(prismforge_gui PRIVATE /W4 /permissive-)
        else()
            target_compile_options(prismforge_gui PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
        endif()
    endif()
endif()

enable_testing()
add_executable(prismforge_tests tests/test_main.cpp)
target_link_libraries(prismforge_tests PRIVATE prismforge_core)
if(PRISMFORGE_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(prismforge_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(prismforge_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endif()
add_test(NAME prismforge_unit COMMAND prismforge_tests)
add_test(NAME prismforge_cli_integration
    COMMAND ${CMAKE_COMMAND}
        -DPRISMFORGE=$<TARGET_FILE:prismforge>
        -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/examples/synthetic.ppm
        -DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/cli-test-output
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_integration.cmake)

if(PRISMFORGE_BUILD_BENCHMARK)
    add_executable(prismforge_bench src/benchmark.cpp)
    target_link_libraries(prismforge_bench PRIVATE prismforge_core)
    if(PRISMFORGE_STRICT_WARNINGS)
        if(MSVC)
            target_compile_options(prismforge_bench PRIVATE /W4 /permissive-)
        else()
            target_compile_options(prismforge_bench PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
        endif()
    endif()
endif()

install(TARGETS prismforge RUNTIME DESTINATION bin)
if(WIN32)
    install(TARGETS prismforge_gui RUNTIME DESTINATION bin)
endif()

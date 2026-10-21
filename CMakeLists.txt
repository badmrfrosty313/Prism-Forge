cmake_minimum_required(VERSION 3.16)
project(PrismForge VERSION 0.1.0 LANGUAGES CXX)

option(PRISMFORGE_STRICT_WARNINGS "Enable strict compiler warnings" ON)
option(PRISMFORGE_BUILD_BENCHMARK "Build the lightweight benchmark" ON)

add_library(prismforge_core
    src/image.cpp
    src/filters.cpp
    src/ppm.cpp)
target_include_directories(prismforge_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(prismforge_core PUBLIC cxx_std_17)
if(PRISMFORGE_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(prismforge_core PRIVATE /W4 /permissive-)
    else()
        target_compile_options(prismforge_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
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

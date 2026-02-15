cmake_minimum_required(VERSION 3.23)
project(csv_median_calculator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)

# =========================
# Boost
# =========================
FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(BOOST_INCLUDE_LIBRARIES program_options accumulators)
set(BOOST_ENABLE_CMAKE ON)
FetchContent_MakeAvailable(Boost)

# =========================
# toml++
# =========================
FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

# =========================
# spdlog
# =========================
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# ======================================================
# CORE LIBRARY (общая логика проекта)
# ======================================================
add_library(core
        src/config_parser.cpp
        src/file_scanner.cpp
        src/csv_reader.cpp
        src/median_calculator.cpp
        src/parallel_processor.cpp
)

target_include_directories(core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(core PUBLIC
        Boost::program_options
        Boost::accumulators
        tomlplusplus::tomlplusplus
        spdlog::spdlog
)

# ======================================================
# MAIN EXECUTABLE
# ======================================================
add_executable(csv_median_calculator
        src/main.cpp
)

target_link_libraries(csv_median_calculator PRIVATE core)

# Copy config to build folder
configure_file(config.toml ${CMAKE_BINARY_DIR}/config.toml COPYONLY)

# ======================================================
# TESTS (optional)
# ======================================================
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.5.0
    )
    FetchContent_MakeAvailable(Catch2)

    add_subdirectory(tests)
endif()

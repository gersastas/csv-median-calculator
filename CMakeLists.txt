## @file CMakeLists.txt
## @brief Конфигурация сборки калькулятора медианы CSV.
## @details Устанавливает стандарт C++23 и настраивает флаги компилятора.

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

add_executable(csv_median_calculator
        src/main.cpp
)
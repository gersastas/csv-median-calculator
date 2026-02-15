/**
* \file main.cpp
 * \brief Точка входа в приложение csv_median_calculator.
 */

#include <iostream>


/**
 * \file main.cpp
 * \brief Точка входа в приложение csv_median_calculator.
 *
 * Обеспечивает инициализацию логирования, парсинг аргументов командной
 * строки, оркестрацию процесса обработки данных и вывод результатов.
 */

#include "config_parser.hpp"
#include "csv_reader.hpp"
#include "file_scanner.hpp"
#include "median_calculator.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <boost/program_options.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

namespace po = boost::program_options;

/**
 * \brief Настраивает систему логирования (spdlog).
 *
 * Использует цветной вывод в консоль с установленным форматом сообщений.
 */
void setup_logging() {
    auto console_sink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("main", console_sink);

    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    // Формат: [Дата Время] [Уровень] Сообщение
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
}

/**
 * \brief Основная функция приложения.
 */

int main() {
    std::cout << "CSV Median Calculator" << std::endl;
}
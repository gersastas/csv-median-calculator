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
 * \brief Парсит аргументы командной строки.
 *
 * Обрабатывает параметры --config (-c), --cfg и --help.
 * Возвращает путь к конфигурационному файлу или nullopt в случае ошибки
 * или запроса справки.
 *
 * \param argc_ Количество аргументов.
 * \param argv_ Массив аргументов.
 * \return Опциональный путь к конфигурационному файлу.
 */
std::optional<std::filesystem::path> parse_arguments(int argc_,
                                                     char* argv_[]) {
    try {
        po::options_description desc("Опции");
        // Добавляем поддерживаемые ключи согласно ТЗ
        desc.add_options()
            ("help,h", "Показать справку")
            ("config,c", po::value<std::string>(),
             "Путь к файлу конфигурации")
            ("cfg", po::value<std::string>(),
             "Путь к файлу конфигурации (алиас)");

        po::variables_map vm;
        po::store(po::parse_command_line(argc_, argv_, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << "CSV Median Calculator v1.0.0\n\n" << desc << "\n";
            return std::nullopt;
        }

        std::filesystem::path config_path;
        // Приоритет: --config, затем --cfg, затем значение по умолчанию
        if (vm.count("config")) {
            config_path = vm["config"].as<std::string>();
        } else if (vm.count("cfg")) {
            config_path = vm["cfg"].as<std::string>();
        } else {
            // По умолчанию ищем config.toml рядом с исполняемым файлом
            config_path = std::filesystem::current_path() / "config.toml";
        }

        return config_path;

    } catch (const po::error& e) {
        spdlog::error("Ошибка парсинга аргументов: {}", e.what());
        return std::nullopt;
    }
}

/**
 * \brief Основная функция приложения.
 */

int main() {
    std::cout << "CSV Median Calculator" << std::endl;
}
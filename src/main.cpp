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

#include "parallel_processor.hpp"
#include "metrics_calculator.hpp"

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
            ("config,c", po::value<std::string>(), "Путь к файлу конфигурации")
            ("cfg", po::value<std::string>(), "Путь к файлу конфигурации (алиас)");

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
int main(int argc, char* argv[]) {
    setup_logging();
    spdlog::info("Запуск приложения csv_median_calculator v1.0.0");

    auto start_time = std::chrono::high_resolution_clock::now();

    // 1. Парсинг аргументов
    auto config_path = parse_arguments(argc, argv);
    // Если путь не возвращен (help или ошибка), завершаем работу.
    // Код возврата 0 для help, 1 для ошибки.
    if (!config_path) {
        return (argc > 1 && std::string(argv[1]) == "--help") ? 0 : 1;
    }

    try {
        // 2. Загрузка конфигурации
        // Используем snake_case имена классов из refactored headers
        auto config = csv_median_calc::config_parser::parse(*config_path);

        // 3. Поиск файлов
        auto csv_files = csv_median_calc::file_scanner::scan_csv_files(config.input_dir, config.filename_masks);

        if (csv_files.empty()) {
            spdlog::error("Не найдено CSV файлов по заданным критериям");
            return 1;
        }

        // ========================================
        // 4. Чтение данных (с выбором режима)
        // ========================================
        std::vector<csv_median_calc::price_record> records;

        if (config.parallel_enabled) {
            // БОНУС 7.1: Параллельная обработка
            spdlog::info("Используется параллельная обработка");
            records = csv_median_calc::parallel_processor::process_parallel(
                csv_files, 
                config.num_threads
            );

            auto stats = csv_median_calc::parallel_processor::get_last_statistics();
            spdlog::info("Статистика: {} файлов за {} мс", 
                        stats.files_processed, stats.duration.count());
        } else {
            // Обычная последовательная обработка
            spdlog::info("Используется последовательная обработка");
            records = csv_median_calc::csv_reader::read_and_merge(csv_files);
        }

        if (records.empty()) {
            spdlog::error("Не удалось прочитать данные из файлов");
            return 1;
        }
        spdlog::info("Прочитано записей: {}", records.size());

        // Базовый расчёт медианы (всегда выполняется)
        spdlog::info("Расчёт медианы...");
        auto results = csv_median_calc::median_calculator::calculate(records);

	    // ========================================
	    // 6. Сохранение результатов
	    // ========================================
        auto output_file = config.output_dir / "median_result.csv";

        if (!csv_median_calc::median_calculator::save_results(results, output_file)) {
            return 1;
        }

        spdlog::info("Базовый результат сохранен: {}", output_file.string());

        // ========================================
        // 6. БОНУС 7.2: Расчёт дополнительных метрик
        // ========================================
        if (config.metrics_enabled && !config.metrics_types.empty()) {
            spdlog::info("Расчёт дополнительных метрик...");

            // Конвертация строк конфига в enum
            std::vector<csv_median_calc::metric_type> requested_metrics;
            for (const auto& str_type : config.metrics_types) {
                if (str_type == "median") requested_metrics.push_back(csv_median_calc::metric_type::median);
                else if (str_type == "mean") requested_metrics.push_back(csv_median_calc::metric_type::mean);
                else if (str_type == "std_dev") requested_metrics.push_back(csv_median_calc::metric_type::std_dev);
                else if (str_type == "p50") requested_metrics.push_back(csv_median_calc::metric_type::p50);
                else if (str_type == "p90") requested_metrics.push_back(csv_median_calc::metric_type::p90);
                else if (str_type == "p95") requested_metrics.push_back(csv_median_calc::metric_type::p95);
                else if (str_type == "p99") requested_metrics.push_back(csv_median_calc::metric_type::p99);
            }

            auto metrics_results = csv_median_calc::metrics_calculator::calculate(records, requested_metrics);
            auto metrics_file = config.output_dir / "metrics_result.csv";

            if (!csv_median_calc::metrics_calculator::save_results(metrics_results, metrics_file, requested_metrics)) {
                spdlog::error("Ошибка сохранения метрик");
                // Не падаем, так как основной результат уже сохранен
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();

        spdlog::info("Записано изменений медианы: {}", results.size());
        spdlog::info("Результат сохранен: {}", output_file.string());
        spdlog::info("Время выполнения: {} мс", duration);
        spdlog::info("Завершение работы");

        return 0;

    } catch (const std::exception& e) {
        spdlog::error("Критическая ошибка: {}", e.what());
        return 1;
    }
}

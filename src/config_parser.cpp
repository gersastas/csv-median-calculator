/**
* @file config_parser.cpp
 * @brief Реализация логики парсинга с использованием toml++ и spdlog.
 */

#include "config_parser.hpp"

#include <toml++/toml.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <format>
#include <stdexcept>

namespace csv_median_calc {

config config_parser::parse(const std::filesystem::path& config_path_) {
    /// @note Сначала проверяем физическое наличие файла, чтобы дать осмысленный текст ошибки
    if (!std::filesystem::exists(config_path_)) {
        throw std::runtime_error(std::format(
            "Конфигурационный файл не найден: {}", config_path_.string()));
    }

    auto table = toml::parse_file(config_path_.string());
    auto main = table["main"];

    /// @warning Параметр 'input' является критически важным для работы приложения
    if (!main || !main["input"]) {
        throw std::runtime_error(
            "В конфигурации отсутствует обязательный параметр input");
    }

    config cfg;
    cfg.input_dir = main["input"].value_or("");

    /// Если output не указан, используем значение по умолчанию "output"
    cfg.output_dir = main["output"].value_or("output");

    /// Обработка опционального списка масок файлов
    if (auto masks = main["filename_mask"].as_array()) {
        for (auto&& m : *masks) {
            cfg.filename_masks.push_back(m.value_or(""));
        }
    }

    //============================================
    // БОНУС 7.1: Чтение секции performance
    //===========================================
    if (auto perf = table["performance"]) {
        cfg.parallel_enabled = perf["parallel"].value_or(false);
        // По умолчанию берем кол-во ядер, если не указано
        cfg.num_threads = perf["num_threads"].value_or(
            std::thread::hardware_concurrency()
        );
    } else {
        cfg.parallel_enabled = false;
    }

    //==================================
    // БОНУС 7.2: Чтение секции metrics
    //==================================
    if (auto metrics = table["metrics"]) {
        cfg.metrics_enabled = metrics["enabled"].value_or(false);

        if (auto types = metrics["types"].as_array()) {
            for (auto&& t : *types) {
                cfg.metrics_types.push_back(t.value_or(""));
            }
        }
    }

    validate_config(cfg);

    spdlog::info("Конфигурация загружена");
    spdlog::info("Входная директория: {}", cfg.input_dir.string());
    spdlog::info("Выходная директория: {}", cfg.output_dir.string());
    spdlog::info("Параллельная обработка: {}",
                 cfg.parallel_enabled ? "ВКЛ" : "ВЫКЛ");

    return cfg;
}

void config_parser::validate_config(const config& config_) {
    /// Проверка доступности входной директории
    if (!std::filesystem::exists(config_.input_dir)) {
        throw std::runtime_error(std::format(
            "Входная директория не существует: {}", config_.input_dir.string()));
    }

    if (!std::filesystem::is_directory(config_.input_dir)) {
        throw std::runtime_error(std::format(
            "Путь не является директорией: {}", config_.input_dir.string()));
    }

    /// Создание выходной директории при необходимости (graceful handling)
    if (!std::filesystem::exists(config_.output_dir)) {
        std::filesystem::create_directories(config_.output_dir);
        spdlog::info("Создана выходная директория: {}",
                     config_.output_dir.string());
    }
}

}  // namespace csv_median_calc
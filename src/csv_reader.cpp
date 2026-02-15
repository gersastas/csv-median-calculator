/**
* @file csv_reader.cpp
 * @brief Реализация парсинга CSV с использованием высокопроизводительных средств C++.
 */

#include "csv_reader.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <string>
#include <vector>

namespace csv_median_calc {

std::vector<price_record> csv_reader::read_and_merge(
    const std::vector<std::filesystem::path>& file_paths_) {

    std::vector<price_record> all_records;

    for (const auto& path : file_paths_) {
        auto records = read_single_file(path);
        /** @note Используем move-итераторы для эффективного переноса данных
         *  из временного вектора файла в общий пул без лишнего копирования строк. */
        all_records.insert(all_records.end(),
                           std::make_move_iterator(records.begin()),
                           std::make_move_iterator(records.end()));
    }

    /// @details Сортировка по возрастанию временной метки получения (receive_ts).
    std::ranges::sort(all_records, [](const auto& a, const auto& b) {
        return a.receive_ts < b.receive_ts;
    });

    spdlog::info("Всего прочитано и отсортировано записей: {}",
                 all_records.size());

    return all_records;
}

std::vector<price_record> csv_reader::read_single_file(
    const std::filesystem::path& file_path_) {

    std::vector<price_record> records;
    std::ifstream file(file_path_);

    if (!file.is_open()) {
        spdlog::error("Не удалось открыть файл: {}", file_path_.string());
        return records;
    }

    std::string line;
    // Пропускаем заголовок файла
    std::getline(file, line);

    size_t line_number = 1;
    while (std::getline(file, line)) {
        ++line_number;
        if (line.empty()) {
            continue;
        }

        auto record = parse_line(line, file_path_, line_number);
        if (record) {
            records.push_back(*record);
        }
    }

    spdlog::info("Из файла {} прочитано {} записей",
                 file_path_.filename().string(), records.size());

    return records;
}

std::optional<price_record> csv_reader::parse_line(
    const std::string& line_,
    const std::filesystem::path& file_path_,
    size_t line_number_) {

    try {
        auto fields = split(line_, ';');

        /// @warning Пропускаем строки, где полей меньше критического минимума (5).
        if (fields.size() < 5) {
            spdlog::debug("Файл {}, строка {}: недостаточно полей ({})",
                          file_path_.filename().string(), line_number_,
                          fields.size());
            return std::nullopt;
        }

        price_record record;

        auto receive_ts = safe_parse<uint64_t>(fields[0]);
        if (!receive_ts) {
            spdlog::debug("Файл {}, строка {}: некорректный receive_ts",
                          file_path_.filename().string(), line_number_);
            return std::nullopt;
        }
        record.receive_ts = *receive_ts;

        if (fields.size() > 1) {
            auto exchange_ts = safe_parse<uint64_t>(fields[1]);
            record.exchange_ts = exchange_ts.value_or(0);
        }

        // Парсинг price (обязательное поле)
        if (fields.size() > 2) {
            auto price = safe_parse<double>(fields[2]);
            if (!price || *price <= 0.0) {
                spdlog::debug("Файл {}, строка {}: некорректная цена '{}'",
                              file_path_.filename().string(), line_number_,
                              fields[2]);
                return std::nullopt;
            }
            record.price = *price;
        } else {
            return std::nullopt;
        }

        // Парсинг quantity
        if (fields.size() > 3) {
            auto quantity = safe_parse<double>(fields[3]);
            record.quantity = quantity.value_or(0.0);
        }

        // Парсинг side (bid/ask)
        if (fields.size() > 4) {
            record.side = fields[4];
        }

        // Парсинг rebuild (флаг есть только в level.csv)
        if (fields.size() > 5) {
            auto rebuild = safe_parse<int>(fields[5]);
            record.rebuild = rebuild.value_or(0);
        }

        return record;

    } catch (const std::exception& e) {
        /// @note Ловим исключения парсинга (например, stod), чтобы не прерывать чтение файла.
        spdlog::debug("Файл {}, строка {}: ошибка парсинга - {}",
                      file_path_.filename().string(), line_number_, e.what());
        return std::nullopt;
    }
}

}  // namespace csv_median_calc

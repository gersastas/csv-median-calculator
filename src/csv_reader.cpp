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
    
}  // namespace csv_median_calc

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
    
}  // namespace csv_median_calc

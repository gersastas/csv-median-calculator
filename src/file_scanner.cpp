/**
 * @file file_scanner.cpp
 * @brief Реализация алгоритмов сканирования с использованием std::ranges.
 */

#include "file_scanner.hpp"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <filesystem>
#include <ranges>
#include <string>
#include <vector>

namespace csv_median_calc {

std::vector<std::filesystem::path> file_scanner::scan_csv_files(
    const std::filesystem::path& directory_,
    const std::vector<std::string>& masks_) {

    std::vector<std::filesystem::path> csv_files;

    try {
        /// @note Используем итератор директории без рекурсии
        for (const auto& entry : std::filesystem::directory_iterator(directory_)) {

            if (!entry.is_regular_file()) continue;

            const auto& path = entry.path();

            // Проверка расширения и применение фильтра масок
            if (path.extension() == ".csv") {
                const std::string filename = path.filename().string();

                if (masks_.empty() || matches_any_mask(filename, masks_)) {
                    csv_files.push_back(path);
                }
            }
        }

        /** @details Сортировка важна для детерминированного порядка обработки данных.
         *  Используем std::ranges для лаконичности синтаксиса C++20. */
        std::ranges::sort(csv_files);

        spdlog::info("Найдено файлов для обработки: {}", csv_files.size());
        return csv_files;

    } catch (const std::filesystem::filesystem_error& e) {
        /// @warning Исключение перехватывается здесь, чтобы не прерывать работу всей программы
        spdlog::error("Ошибка при сканировании директории: {}", e.what());
        return {};
    }
}

bool file_scanner::matches_any_mask(const std::string& filename_,
                                    const std::vector<std::string>& masks_) {
    /**
     * @details Использует алгоритм any_of. Проверка идет по вхождению подстроки
     * (substring match), а не по строгому regex или glob.
     */
    return std::ranges::any_of(masks_, [&filename_](const std::string& mask) {
        return filename_.find(mask) != std::string::npos;
    });
}

}  // namespace csv_median_calc

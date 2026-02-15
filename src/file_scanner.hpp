/**
 * \file file_scanner.hpp
 * \brief Модуль сканирования директорий для поиска CSV-файлов.
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace csv_median_calc {

/**
 * \brief Класс для поиска файлов по заданным критериям.
 */
class file_scanner {
public:
    /**
     * \brief Сканирует директорию на наличие CSV-файлов.
     *
     * Фильтрует файлы по расширению и соответствию маскам имен.
     * Если список масок пуст, возвращаются все CSV-файлы директории.
     *
     * \param directory_ Путь к директории для сканирования.
     * \param filename_masks_ Список подстрок для фильтрации имен.
     * \return Отсортированный список путей к найденным файлам.
     */
    static std::vector<std::filesystem::path> scan_csv_files(
        const std::filesystem::path& directory_,
        const std::vector<std::string>& filename_masks_);

private:
    /**
     * \brief Проверяет, содержит ли имя файла одну из указанных масок.
     */
    static bool matches_any_mask(const std::string& filename_,
                                 const std::vector<std::string>& masks_);
};

}  // namespace csv_median_calc
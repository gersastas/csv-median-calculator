/**
 * \file csv_reader.hpp
 * \brief Модуль чтения и объединения биржевых данных из CSV-файлов.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace csv_median_calc {

/**
 * \brief Структура записи о торговой операции или котировке.
 */
struct price_record {
    uint64_t receive_ts;
    uint64_t exchange_ts;
    double price;
    double quantity;
    std::string side;
    int rebuild;

    /**
     * \brief Оператор сравнения для сортировки по времени получения.
     */
    auto operator<=>(const price_record&) const = default;
};

/**
 * \brief Класс для чтения, парсинга и слияния CSV-файлов.
 */
class csv_reader {
public:
    /**
     * \brief Читает несколько CSV-файлов и объединяет их в один список.
     *
     * Данные из всех файлов сливаются, а затем сортируются по полю
     * receive_ts для обеспечения хронологического порядка.
     *
     * \param file_paths_ Список путей к файлам для обработки.
     * \return Вектор записей, отсортированный по времени.
     */
    static std::vector<price_record> read_and_merge(
        const std::vector<std::filesystem::path>& file_paths_);

private:
    /**
     * \brief Читает и парсит один CSV-файл.
     *
     * \param file_path_ Путь к файлу.
     * \return Вектор прочитанных записей.
     */
    static std::vector<price_record> read_single_file(
        const std::filesystem::path& file_path_);

    /**
     * \brief Парсит одну строку CSV в структуру price_record.
     *
     * Осуществляет валидацию данных и преобразование типов.
     *
     * \param line_ Строка текста.
     * \param file_path_ Путь к файлу (для логирования ошибок).
     * \param line_number_ Номер строки (для логирования ошибок).
     * \return std::optional с записью или nullopt в случае ошибки парсинга.
     */
    static std::optional<price_record> parse_line(
        const std::string& line_,
        const std::filesystem::path& file_path_,
        size_t line_number_);

    /**
     * \brief Разделяет строку по разделителю.
     */
    static std::vector<std::string> split(const std::string& str_,
                                          char delimiter_);

    /**
     * \brief Безопасно парсит строку в числовой тип.
     */
    template<typename T>
    static std::optional<T> safe_parse(const std::string& str_);
};

}  // namespace csv_median_calc
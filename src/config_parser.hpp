/**
* \file config_parser.hpp
 * \brief Модуль парсинга конфигурационного файла TOML.
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace csv_median_calc {

    /**
     * \brief Структура для хранения параметров конфигурации приложения.
     */
    struct config {
        std::filesystem::path input_dir;
        std::filesystem::path output_dir;
        std::vector<std::string> filename_masks;
    };

    /**
     * \brief Класс для парсинга и валидации конфигурационного файла.
     */
    class config_parser {
    public:
        /**
         * \brief Загружает конфигурацию из TOML-файла.
         *
         * Читает файл, извлекает параметры и проводит валидацию путей.
         * При отсутствии необязательных полей устанавливает значения по умолчанию.
         *
         * \param config_path_ Путь к файлу конфигурации.
         * \return Объект config с заполненными параметрами.
         * \throws std::runtime_error Если файл не найден, невалиден или
         *         отсутствуют обязательные параметры.
         */
        static config parse(const std::filesystem::path& config_path_);

    private:
        /**
         * \brief Проверяет корректность путей в конфигурации.
         *
         * Проверяет существование входной директории и создает выходную,
         * если она отсутствует.
         *
         * \param config_ Конфигурация для проверки.
         * \throws std::runtime_error При ошибках доступа к файловой системе.
         */
        static void validate_config(const config& config_);
    };

}  // namespace csv_median_calc
/**
 * \file median_calculator.hpp
 * \brief Модуль инкрементального расчета медианы и сохранения результатов.
 */

#pragma once

#include <filesystem>
#include <vector>

// Предварительное объявление структуры записи (определена в csv_reader.hpp)
// или можно подключить заголовок, если используется полная зависимость.
// Для чистоты интерфейса используем предварительное объявление, 
// но в реализации понадобится полный тип.
// В данном контексте предполагаем, что csv_reader.hpp включен в .cpp

namespace csv_median_calc {

struct price_record;  // forward declaration

/**
 * \brief Структура результата расчета медианы.
 */
struct median_result {
    uint64_t _receive_ts;
    double _median;
};

/**
 * \brief Класс для вычисления медианы потоком и сохранения результатов.
 */
class median_calculator {
public:
    /**
     * \brief Вычисляет медиану инкрементально для списка записей.
     *
     * Использует гибридный подход: Boost.Accumulators для вспомогательной
     * статистики и собственный алгоритм (two-heaps) для точной медианы.
     *
     * \param records_ Вектор записей с ценами.
     * \return Вектор результатов, содержащий только моменты изменения медианы.
     */
    static std::vector<median_result> calculate(
        const std::vector<price_record>& records_);

    /**
     * \brief Сохраняет результаты расчета в CSV-файл.
     *
     * \param results_ Вектор результатов для сохранения.
     * \param output_path_ Путь к выходному файлу.
     * \return true при успешной записи, false при ошибке.
     */
    static bool save_results(
        const std::vector<median_result>& results_,
        const std::filesystem::path& output_path_);
};

}  // namespace csv_median_calc
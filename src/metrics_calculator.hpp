/**
 * \file metrics_calculator.hpp
 * \brief БОНУС 7.2: Расчёт дополнительных метрик с Boost.Accumulators
 */

#pragma once

#include "csv_reader.hpp"
#include <map>
#include <vector>
#include <string>

namespace csv_median_calc {

/**
 * \brief Типы доступных метрик
 */
enum class metric_type {
    median,
    mean,
    std_dev,
    p50, p90, p95, p99
};

/**
 * \brief Результат расчёта метрик для конкретного timestamp
 */
struct metrics_result {
    uint64_t receive_ts;
    std::map<metric_type, double> values;
};

/**
 * \brief Калькулятор дополнительных метрик (БОНУС 7.2)
 */
class metrics_calculator {
public:
    /**
     * \brief Вычисляет выбранные метрики для всех записей
     * \param records_ Входные данные
     * \param metrics_ Список метрик для расчёта
     * \return Результаты расчёта для каждого timestamp
     */
    static std::vector<metrics_result> calculate(
        const std::vector<price_record>& records_,
        const std::vector<metric_type>& metrics_
    );
    
    /**
     * \brief Сохраняет метрики в CSV файл
     */
    static bool save_results(
        const std::vector<metrics_result>& results_,
        const std::filesystem::path& output_path_,
        const std::vector<metric_type>& metrics_
    );
    
private:
    static double calculate_percentile(
        const std::vector<double>& sorted_data_,
        double percentile_
    );
    
    static std::string metric_name(metric_type type_);
};

} // namespace csv_median_calc

/**
 * \file metrics_calculator.cpp
 * \brief Реализация расчёта метрик с использованием Boost.Accumulators
 */

#include "metrics_calculator.hpp"
#include <spdlog/spdlog.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip> // <--- Добавлено для std::fixed

namespace csv_median_calc {

using namespace boost::accumulators;

std::vector<metrics_result> metrics_calculator::calculate(
    const std::vector<price_record>& records_,
    const std::vector<metric_type>& metrics_
) {
    if (records_.empty() || metrics_.empty()) {
        return {};
    }

    spdlog::info("БОНУС 7.2: Расчёт метрик для {} записей", records_.size());

    std::vector<metrics_result> results;
    // Резервируем меньше места, так как файл метрик может быть большим
    results.reserve(records_.size());

    accumulator_set<double, stats<tag::mean, tag::variance>> acc;
    std::vector<double> all_prices;
    all_prices.reserve(records_.size());

    for (const auto& record : records_) {
        acc(record.price);
        all_prices.push_back(record.price);

        metrics_result result;
        result.receive_ts = record.receive_ts;

        // Вычисляем каждую метрику
        for (const auto& metric : metrics_) {
            switch (metric) {
                case metric_type::mean:
                    result.values[metric] = mean(acc);
                    break;

                case metric_type::std_dev:
                    // Variance в Boost дает момент, требуем корень
                    if (all_prices.size() > 1) {
                        result.values[metric] = std::sqrt(variance(acc));
                    } else {
                        result.values[metric] = 0.0;
                    }
                    break;

                // Для перцентилей используем точный расчет (сортировка копии)
                // ВНИМАНИЕ: Это O(N^2 log N), медленно для больших данных.
                // Для продакшена лучше использовать P^2 алгоритм или гистограммы.
                case metric_type::median:
                case metric_type::p50: {
                    auto sorted = all_prices;
                    std::sort(sorted.begin(), sorted.end());
                    result.values[metric] = calculate_percentile(sorted, 0.5);
                    break;
                }

                case metric_type::p90: {
                    auto sorted = all_prices;
                    std::sort(sorted.begin(), sorted.end());
                    result.values[metric] = calculate_percentile(sorted, 0.9);
                    break;
                }

                case metric_type::p95: {
                    auto sorted = all_prices;
                    std::sort(sorted.begin(), sorted.end());
                    result.values[metric] = calculate_percentile(sorted, 0.95);
                    break;
                }

                case metric_type::p99: {
                    auto sorted = all_prices;
                    std::sort(sorted.begin(), sorted.end());
                    result.values[metric] = calculate_percentile(sorted, 0.99);
                    break;
                }
            }
        }

        results.push_back(result);
    }

    return results;
}

double metrics_calculator::calculate_percentile(
    const std::vector<double>& sorted_data_,
    double percentile_
) {
    if (sorted_data_.empty()) return 0.0;
    if (sorted_data_.size() == 1) return sorted_data_[0];

    // Линейная интерполяция
    double index = percentile_ * (sorted_data_.size() - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));

    if (lower == upper || upper >= sorted_data_.size()) {
        return sorted_data_[lower];
    }

    double weight = index - lower;
    return sorted_data_[lower] * (1.0 - weight) + sorted_data_[upper] * weight;
}

bool metrics_calculator::save_results(
    const std::vector<metrics_result>& results_,
    const std::filesystem::path& output_path_,
    const std::vector<metric_type>& metrics_
) {
    std::ofstream file(output_path_);
    if (!file.is_open()) {
        spdlog::error("Не удалось создать файл: {}", output_path_.string());
        return false;
    }

    // Заголовок
    file << "receive_ts";
    for (const auto& metric : metrics_) {
        file << ";" << metric_name(metric);
    }
    file << "\n";

    // Данные
    for (const auto& result : results_) {
        file << result.receive_ts;
        for (const auto& metric : metrics_) {
            // Используем поиск по ключу, если метрика отсутствует (например std_dev на 1 элементе)
            auto it = result.values.find(metric);
            if (it != result.values.end()) {
                 file << ";" << std::fixed << std::setprecision(8) << it->second;
            } else {
                 file << ";0.0"; // Fallback
            }
        }
        file << "\n";
    }

    spdlog::info("Метрики сохранены: {}", output_path_.string());
    return true;
}

std::string metrics_calculator::metric_name(metric_type type_) {
    switch (type_) {
        case metric_type::median: return "median";
        case metric_type::mean: return "mean";
        case metric_type::std_dev: return "std_dev";
        case metric_type::p50: return "p50";
        case metric_type::p90: return "p90";
        case metric_type::p95: return "p95";
        case metric_type::p99: return "p99";
        default: return "unknown";
    }
}

} // namespace csv_median_calc
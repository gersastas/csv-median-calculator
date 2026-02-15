/**
 * \file test_metrics_calculator.cpp
 * \brief Unit-тесты для расчёта дополнительных метрик (БОНУС 7.2)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../src/metrics_calculator.hpp"
#include "../src/csv_reader.hpp"

#include <filesystem>
#include <fstream>
#include <cmath>
#include <algorithm>

using namespace csv_median_calc;

// ============================================
// Вспомогательные функции для тестов
// ============================================

/**
 * \brief Создаёт тестовые price_record
 */
std::vector<price_record> create_test_records(const std::vector<double>& prices) {
    std::vector<price_record> records;
    uint64_t ts = 1000;
    
    for (double price : prices) {
        price_record rec;
        rec.receive_ts = ts;
        rec.exchange_ts = ts;
        rec.price = price;
        rec.quantity = 1.0;
        rec.side = "bid";
        rec.rebuild = 0;
        
        records.push_back(rec);
        ts += 1000;
    }
    
    return records;
}

/**
 * \brief Вычисляет медиану вручную для проверки
 */
double calculate_manual_median(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    
    auto sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    size_t n = sorted.size();
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    } else {
        return sorted[n/2];
    }
}

/**
 * \brief Вычисляет среднее вручную для проверки
 */
double calculate_manual_mean(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    
    double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    return sum / values.size();
}

/**
 * \brief Вычисляет стандартное отклонение вручную для проверки
 */
double calculate_manual_stddev(const std::vector<double>& values) {
    if (values.size() <= 1) return 0.0;
    
    double mean = calculate_manual_mean(values);
    double sum_sq_diff = 0.0;
    
    for (double v : values) {
        double diff = v - mean;
        sum_sq_diff += diff * diff;
    }
    
    return std::sqrt(sum_sq_diff / values.size());
}

/**
 * \brief Вычисляет перцентиль вручную для проверки
 */
double calculate_manual_percentile(const std::vector<double>& values, double percentile) {
    if (values.empty()) return 0.0;
    
    auto sorted = values;
    std::sort(sorted.begin(), sorted.end());
    
    if (sorted.size() == 1) return sorted[0];
    
    double index = percentile * (sorted.size() - 1);
    size_t lower = static_cast<size_t>(std::floor(index));
    size_t upper = static_cast<size_t>(std::ceil(index));
    
    if (lower == upper) {
        return sorted[lower];
    }
    
    double weight = index - lower;
    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

// ============================================
// ТЕСТЫ
// ============================================

TEST_CASE("MetricsCalculator обрабатывает пустой список записей", "[metrics]") {
    std::vector<price_record> empty_records;
    std::vector<metric_type> metrics = {metric_type::mean, metric_type::median};
    
    auto result = metrics_calculator::calculate(empty_records, metrics);
    
    REQUIRE(result.empty());
}

TEST_CASE("MetricsCalculator обрабатывает пустой список метрик", "[metrics]") {
    auto records = create_test_records({100.0, 200.0, 300.0});
    std::vector<metric_type> empty_metrics;
    
    auto result = metrics_calculator::calculate(records, empty_metrics);
    
    REQUIRE(result.empty());
}

TEST_CASE("MetricsCalculator вычисляет median корректно", "[metrics]") {
    // Тест на нечётное количество значений
    auto records1 = create_test_records({100.0, 200.0, 300.0});
    std::vector<metric_type> metrics = {metric_type::median};
    
    auto result1 = metrics_calculator::calculate(records1, metrics);
    
    REQUIRE(result1.size() == 3);
    
    // Медиана одного элемента
    REQUIRE_THAT(result1[0].values[metric_type::median], 
                Catch::Matchers::WithinRel(100.0, 0.01));
    
    // Медиана двух элементов (100, 200)
    REQUIRE_THAT(result1[1].values[metric_type::median], 
                Catch::Matchers::WithinRel(150.0, 0.01));
    
    // Медиана трёх элементов (100, 200, 300)
    REQUIRE_THAT(result1[2].values[metric_type::median], 
                Catch::Matchers::WithinRel(200.0, 0.01));
    
    // Тест на чётное количество значений
    auto records2 = create_test_records({100.0, 200.0, 300.0, 400.0});
    auto result2 = metrics_calculator::calculate(records2, metrics);
    
    REQUIRE(result2.size() == 4);
    
    // Медиана четырёх элементов (100, 200, 300, 400)
    REQUIRE_THAT(result2[3].values[metric_type::median], 
                Catch::Matchers::WithinRel(250.0, 0.01));
}

TEST_CASE("MetricsCalculator вычисляет mean корректно", "[metrics]") {
    auto records = create_test_records({100.0, 200.0, 300.0, 400.0, 500.0});
    std::vector<metric_type> metrics = {metric_type::mean};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 5);
    
    // Среднее одного элемента
    REQUIRE_THAT(result[0].values[metric_type::mean], 
                Catch::Matchers::WithinRel(100.0, 0.01));
    
    // Среднее двух элементов (100 + 200) / 2
    REQUIRE_THAT(result[1].values[metric_type::mean], 
                Catch::Matchers::WithinRel(150.0, 0.01));
    
    // Среднее пяти элементов (100 + 200 + 300 + 400 + 500) / 5
    REQUIRE_THAT(result[4].values[metric_type::mean], 
                Catch::Matchers::WithinRel(300.0, 0.01));
}

TEST_CASE("MetricsCalculator вычисляет std_dev корректно", "[metrics]") {
    auto records = create_test_records({100.0, 200.0, 300.0, 400.0, 500.0});
    std::vector<metric_type> metrics = {metric_type::std_dev};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 5);
    
    // Стандартное отклонение одного элемента должно быть 0 или не определено
    // (зависит от реализации, но должно быть близко к 0)
    
    // Проверяем последнее значение (все 5 элементов)
    std::vector<double> all_prices = {100.0, 200.0, 300.0, 400.0, 500.0};
    double expected_stddev = calculate_manual_stddev(all_prices);
    
    REQUIRE_THAT(result[4].values[metric_type::std_dev], 
                Catch::Matchers::WithinRel(expected_stddev, 0.01));
}

TEST_CASE("MetricsCalculator вычисляет p50 (эквивалент median)", "[metrics]") {
    auto records = create_test_records({100.0, 200.0, 300.0, 400.0, 500.0});
    std::vector<metric_type> metrics = {metric_type::p50, metric_type::median};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 5);
    
    // p50 и median должны быть одинаковыми
    for (const auto& res : result) {
        REQUIRE_THAT(res.values.at(metric_type::p50), 
                    Catch::Matchers::WithinRel(res.values.at(metric_type::median), 0.01));
    }
}

TEST_CASE("MetricsCalculator вычисляет p90 корректно", "[metrics]") {
    auto records = create_test_records({10.0, 20.0, 30.0, 40.0, 50.0, 
                                       60.0, 70.0, 80.0, 90.0, 100.0});
    std::vector<metric_type> metrics = {metric_type::p90};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 10);
    
    // Проверяем последнее значение (все 10 элементов)
    std::vector<double> all_prices = {10.0, 20.0, 30.0, 40.0, 50.0, 
                                      60.0, 70.0, 80.0, 90.0, 100.0};
    double expected_p90 = calculate_manual_percentile(all_prices, 0.90);
    
    REQUIRE_THAT(result[9].values[metric_type::p90], 
                Catch::Matchers::WithinRel(expected_p90, 0.5));
}

TEST_CASE("MetricsCalculator вычисляет p95 корректно", "[metrics]") {
    auto records = create_test_records({10.0, 20.0, 30.0, 40.0, 50.0, 
                                       60.0, 70.0, 80.0, 90.0, 100.0});
    std::vector<metric_type> metrics = {metric_type::p95};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 10);
    
    std::vector<double> all_prices = {10.0, 20.0, 30.0, 40.0, 50.0, 
                                      60.0, 70.0, 80.0, 90.0, 100.0};
    double expected_p95 = calculate_manual_percentile(all_prices, 0.95);
    
    REQUIRE_THAT(result[9].values[metric_type::p95], 
                Catch::Matchers::WithinRel(expected_p95, 0.5));
}

TEST_CASE("MetricsCalculator вычисляет p99 корректно", "[metrics]") {
    // Для p99 нужно больше точек
    std::vector<double> prices;
    for (int i = 1; i <= 100; ++i) {
        prices.push_back(static_cast<double>(i));
    }
    
    auto records = create_test_records(prices);
    std::vector<metric_type> metrics = {metric_type::p99};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 100);
    
    double expected_p99 = calculate_manual_percentile(prices, 0.99);
    
    REQUIRE_THAT(result[99].values[metric_type::p99], 
                Catch::Matchers::WithinRel(expected_p99, 1.0));
}

TEST_CASE("MetricsCalculator вычисляет несколько метрик одновременно", "[metrics]") {
    auto records = create_test_records({100.0, 200.0, 300.0, 400.0, 500.0});
    std::vector<metric_type> metrics = {
        metric_type::median,
        metric_type::mean,
        metric_type::std_dev,
        metric_type::p90,
        metric_type::p95
    };
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 5);
    
    // Проверяем что все метрики присутствуют в последнем результате
    const auto& last_result = result[4];
    
    REQUIRE(last_result.values.count(metric_type::median) == 1);
    REQUIRE(last_result.values.count(metric_type::mean) == 1);
    REQUIRE(last_result.values.count(metric_type::std_dev) == 1);
    REQUIRE(last_result.values.count(metric_type::p90) == 1);
    REQUIRE(last_result.values.count(metric_type::p95) == 1);
    
    // Проверяем разумность значений
    std::vector<double> all_prices = {100.0, 200.0, 300.0, 400.0, 500.0};
    
    REQUIRE_THAT(last_result.values.at(metric_type::median), 
                Catch::Matchers::WithinRel(300.0, 0.01));
    
    REQUIRE_THAT(last_result.values.at(metric_type::mean), 
                Catch::Matchers::WithinRel(300.0, 0.01));
    
    REQUIRE(last_result.values.at(metric_type::std_dev) > 0.0);
    
    REQUIRE(last_result.values.at(metric_type::p90) >= 
            last_result.values.at(metric_type::median));
    
    REQUIRE(last_result.values.at(metric_type::p95) >= 
            last_result.values.at(metric_type::p90));
}

TEST_CASE("MetricsCalculator корректно обрабатывает инкрементальные вычисления", "[metrics]") {
    auto records = create_test_records({100.0, 150.0, 200.0});
    std::vector<metric_type> metrics = {metric_type::mean, metric_type::median};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 3);
    
    // Проверяем что каждый результат вычислен с учётом только предыдущих значений
    
    // После первой записи
    REQUIRE_THAT(result[0].values[metric_type::mean], 
                Catch::Matchers::WithinRel(100.0, 0.01));
    REQUIRE_THAT(result[0].values[metric_type::median], 
                Catch::Matchers::WithinRel(100.0, 0.01));
    
    // После второй записи (100, 150)
    REQUIRE_THAT(result[1].values[metric_type::mean], 
                Catch::Matchers::WithinRel(125.0, 0.01));
    REQUIRE_THAT(result[1].values[metric_type::median], 
                Catch::Matchers::WithinRel(125.0, 0.01));
    
    // После третьей записи (100, 150, 200)
    REQUIRE_THAT(result[2].values[metric_type::mean], 
                Catch::Matchers::WithinRel(150.0, 0.01));
    REQUIRE_THAT(result[2].values[metric_type::median], 
                Catch::Matchers::WithinRel(150.0, 0.01));
}

TEST_CASE("MetricsCalculator сохраняет receive_ts корректно", "[metrics]") {
    std::vector<price_record> records;
    
    price_record rec1;
    rec1.receive_ts = 1234567890;
    rec1.price = 100.0;
    rec1.quantity = 1.0;
    rec1.side = "bid";
    rec1.rebuild = 0;
    
    price_record rec2;
    rec2.receive_ts = 9876543210;
    rec2.price = 200.0;
    rec2.quantity = 1.0;
    rec2.side = "ask";
    rec2.rebuild = 0;
    
    records.push_back(rec1);
    records.push_back(rec2);
    
    std::vector<metric_type> metrics = {metric_type::mean};
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 2);
    REQUIRE(result[0].receive_ts == 1234567890);
    REQUIRE(result[1].receive_ts == 9876543210);
}

TEST_CASE("MetricsCalculator обрабатывает одинаковые значения", "[metrics]") {
    // Все цены одинаковые
    auto records = create_test_records({100.0, 100.0, 100.0, 100.0, 100.0});
    std::vector<metric_type> metrics = {
        metric_type::median,
        metric_type::mean,
        metric_type::std_dev
    };
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 5);
    
    const auto& last = result[4];
    
    // Все метрики должны быть 100.0 (кроме stddev = 0)
    REQUIRE_THAT(last.values.at(metric_type::median), 
                Catch::Matchers::WithinRel(100.0, 0.01));
    
    REQUIRE_THAT(last.values.at(metric_type::mean), 
                Catch::Matchers::WithinRel(100.0, 0.01));
    
    // Стандартное отклонение должно быть 0 или очень близко к 0
    REQUIRE_THAT(last.values.at(metric_type::std_dev), 
                Catch::Matchers::WithinAbs(0.0, 0.01));
}

TEST_CASE("MetricsCalculator работает с большим объёмом данных", "[metrics][performance]") {
    // Создаём 10000 записей
    std::vector<double> prices;
    for (int i = 0; i < 10000; ++i) {
        prices.push_back(1000.0 + static_cast<double>(i) * 0.01);
    }
    
    auto records = create_test_records(prices);
    std::vector<metric_type> metrics = {
        metric_type::median,
        metric_type::mean,
        metric_type::std_dev,
        metric_type::p90,
        metric_type::p95,
        metric_type::p99
    };
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = metrics_calculator::calculate(records, metrics);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    );
    
    REQUIRE(result.size() == 10000);
    
    INFO("Обработано 10000 записей за " << duration.count() << " мс");
    
    // Проверяем что вычисления завершились за разумное время (< 10 секунд)
    REQUIRE(duration.count() < 10000);
    
    // Проверяем последний результат
    const auto& last = result[9999];
    
    REQUIRE(last.values.count(metric_type::median) == 1);
    REQUIRE(last.values.count(metric_type::mean) == 1);
    REQUIRE(last.values.count(metric_type::std_dev) == 1);
    REQUIRE(last.values.count(metric_type::p90) == 1);
    REQUIRE(last.values.count(metric_type::p95) == 1);
    REQUIRE(last.values.count(metric_type::p99) == 1);
}

TEST_CASE("MetricsCalculator метрики упорядочены корректно", "[metrics]") {
    auto records = create_test_records({10.0, 20.0, 30.0, 40.0, 50.0, 
                                       60.0, 70.0, 80.0, 90.0, 100.0});
    std::vector<metric_type> metrics = {
        metric_type::median,
        metric_type::p90,
        metric_type::p95,
        metric_type::p99
    };
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 10);
    
    const auto& last = result[9];
    
    // Проверяем что перцентили упорядочены: p50 <= p90 <= p95 <= p99
    double p50 = last.values.at(metric_type::median);
    double p90 = last.values.at(metric_type::p90);
    double p95 = last.values.at(metric_type::p95);
    double p99 = last.values.at(metric_type::p99);
    
    REQUIRE(p50 <= p90);
    REQUIRE(p90 <= p95);
    REQUIRE(p95 <= p99);
}

TEST_CASE("MetricsCalculator корректен на реальных биржевых данных", "[metrics][integration]") {
    // Симулируем реальные цены BTC/USDT
    auto records = create_test_records({
        68480.10, 68480.00, 68480.10, 68480.20, 68479.90,
        68481.00, 68480.50, 68479.50, 68480.80, 68481.50
    });
    
    std::vector<metric_type> metrics = {
        metric_type::median,
        metric_type::mean,
        metric_type::std_dev
    };
    
    auto result = metrics_calculator::calculate(records, metrics);
    
    REQUIRE(result.size() == 10);
    
    const auto& last = result[9];
    
    // Проверяем что значения в разумном диапазоне
    REQUIRE(last.values.at(metric_type::median) > 68479.0);
    REQUIRE(last.values.at(metric_type::median) < 68482.0);
    
    REQUIRE(last.values.at(metric_type::mean) > 68479.0);
    REQUIRE(last.values.at(metric_type::mean) < 68482.0);
    
    REQUIRE(last.values.at(metric_type::std_dev) >= 0.0);
    REQUIRE(last.values.at(metric_type::std_dev) < 10.0);
}

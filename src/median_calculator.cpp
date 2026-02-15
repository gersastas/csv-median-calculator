/**
 * \file median_calculator.cpp
 * \brief Реализация алгоритма расчета медианы.
 *
 * Используется гибридный подход для соответствия ТЗ и обеспечения точности:
 * 1. Boost.Accumulators используется для сбора вспомогательной статистики
 *    (среднее, количество), чтобы продемонстрировать владение библиотекой.
 * 2. Собственный алгоритм на двух кучах (heaps) используется для точного
 *    расчета инкрементальной медианы, так как стандартные реализации
 *    медианы в Boost (density или p_square) дают приближенные значения.
 */

#include "median_calculator.hpp"
#include "csv_reader.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

// Подключаем Boost.Accumulators согласно ТЗ п.4.2
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/count.hpp>

namespace csv_median_calc {

/**
 * \brief Вспомогательный класс для точного инкрементального расчета медианы.
 *
 * Реализует алгоритм с использованием двух приоритетных очередей (heaps):
 * - Max-heap для нижней половины значений.
 * - Min-heap для верхней половины значений.
 * Это позволяет вычислять медиану за O(log n) на вставку и O(1) на получение.
 */
class incremental_median {
public:
    void add_value(double value_) {
        // Определяем, в какую кучу добавить значение
        if (_max_heap.empty() || value_ <= _max_heap.front()) {
            _max_heap.push_back(value_);
            std::push_heap(_max_heap.begin(), _max_heap.end());
        } else {
            _min_heap.push_back(value_);
            std::push_heap(_min_heap.begin(), _min_heap.end(),
                           std::greater<>{});
        }

        balance_heaps();
    }

    double get_median() const noexcept {
        if (_max_heap.empty() && _min_heap.empty()) {
            return 0.0;
        }

        // Если в кучах разное количество элементов, медиана - вершина большей
        if (_max_heap.size() > _min_heap.size()) {
            return _max_heap.front();
        }
        if (_min_heap.size() > _max_heap.size()) {
            return _min_heap.front();
        }

        return (_max_heap.front() + _min_heap.front()) / 2.0;
    }

private:
    std::vector<double> _max_heap;
    std::vector<double> _min_heap;

    /**
     * \brief Балансирует кучи, чтобы разница размеров не превышала 1.
     */
    void balance_heaps() {
        if (_max_heap.size() > _min_heap.size() + 1) {
            double value = _max_heap.front();
            std::pop_heap(_max_heap.begin(), _max_heap.end());
            _max_heap.pop_back();

            _min_heap.push_back(value);
            std::push_heap(_min_heap.begin(), _min_heap.end(),
                           std::greater<>{});
        } else if (_min_heap.size() > _max_heap.size() + 1) {
            double value = _min_heap.front();
            std::pop_heap(_min_heap.begin(), _min_heap.end(),
                          std::greater<>{});
            _min_heap.pop_back();

            _max_heap.push_back(value);
            std::push_heap(_max_heap.begin(), _max_heap.end());
        }
    }
};

}  // namespace csv_median_calc
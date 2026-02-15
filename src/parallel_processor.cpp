/**
 * \file parallel_processor.cpp
 * \brief Реализация многопоточной обработки CSV файлов.
 */

#include "parallel_processor.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace csv_median_calc {

// Определение статических членов класса
parallel_processor::statistics parallel_processor::last_stats_;
std::mutex parallel_processor::stats_mutex_;

std::vector<price_record> parallel_processor::process_parallel(
    const std::vector<std::filesystem::path>& files_,
    size_t num_threads_
) {
    auto start = std::chrono::high_resolution_clock::now();

    if (files_.empty()) {
        spdlog::warn("Нет файлов для параллельной обработки");
        return {};
    }

    num_threads_ = std::min(num_threads_, files_.size());
    num_threads_ = std::max(size_t{1}, num_threads_);

    spdlog::info("БОНУС 7.1: Параллельная обработка {} файлов в {} потоках",
                 files_.size(), num_threads_);

    std::vector<std::future<std::vector<price_record>>> futures;
    futures.reserve(files_.size());

    for (const auto& file : files_) {
        futures.push_back(
            std::async(
                std::launch::async,
                read_file_threadsafe,
                file
            )
        );
    }

    std::vector<std::vector<price_record>> results;
    results.reserve(futures.size());

    // Исправление warning: переменная total_records была не нужна,
    // так как мы логируем merged.size() в конце.
    for (auto& future : futures) {
        try {
            auto records = future.get();
            results.push_back(std::move(records));
        } catch (const std::exception& e) {
            spdlog::error("Ошибка в потоке: {}", e.what());
        }
    }

    auto merged = merge_sorted_results(results);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    );

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        last_stats_ = {
            .files_processed = files_.size(),
            .total_records = merged.size(),
            .duration = duration
        };
    }

    spdlog::info("Параллельная обработка завершена за {} мс",
                 duration.count());

    return merged;
}

std::vector<price_record> parallel_processor::read_file_threadsafe(
    const std::filesystem::path& path_
) {
    try {
        return csv_reader::read_single_file(path_);
    } catch (const std::exception& e) {
        spdlog::error("Ошибка чтения файла {}: {}",
                      path_.string(), e.what());
        return {};
    }
}

std::vector<price_record> parallel_processor::merge_sorted_results(
    std::vector<std::vector<price_record>>& results_
) {
    if (results_.empty()) return {};

    size_t total_size = 0;
    for (const auto& res : results_) {
        total_size += res.size();
    }

    std::vector<price_record> merged;
    merged.reserve(total_size);

    for (auto& res : results_) {
        merged.insert(
            merged.end(),
            std::make_move_iterator(res.begin()),
            std::make_move_iterator(res.end())
        );
    }

    std::sort(merged.begin(), merged.end(),
        [](const price_record& a, const price_record& b) {
            return a.receive_ts < b.receive_ts;
        }
    );

    return merged;
}

parallel_processor::statistics parallel_processor::get_last_statistics()
    noexcept {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return last_stats_;
}

}  // namespace csv_median_calc
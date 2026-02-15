/**
* \file parallel_processor.hpp
 * \brief БОНУС 7.1: Многопоточная обработка CSV файлов.
 */

#pragma once

#include "csv_reader.hpp"
#include <filesystem>
#include <vector>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>

namespace csv_median_calc {

    class parallel_processor {
    public:
        struct statistics {
            size_t files_processed{0};
            size_t total_records{0};
            std::chrono::milliseconds duration{0};
        };

        static std::vector<price_record> process_parallel(
            const std::vector<std::filesystem::path>& files_,
            size_t num_threads_ = std::thread::hardware_concurrency()
        );

        static statistics get_last_statistics() noexcept;

    private:
        static std::vector<price_record> read_file_threadsafe(
            const std::filesystem::path& path_
        );

        static std::vector<price_record> merge_sorted_results(
            std::vector<std::vector<price_record>>& results_
        );

        // Убираем inline, просто объявляем
        static statistics last_stats_;
        static std::mutex stats_mutex_;
    };

}  // namespace csv_median_calc
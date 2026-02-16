/**
 * \file streaming_processor.cpp
 * \brief Реализация потоковой обработки
 */

#include "streaming_processor.hpp"
#include "csv_reader.hpp"
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <iomanip> // <--- Добавлено

namespace csv_median_calc {

void streaming_processor::process_streaming(
    const std::filesystem::path& input_file_,
    const std::filesystem::path& output_file_,
    size_t buffer_size_
) {
    spdlog::info("БОНУС 7.3: Потоковая обработка файла {}",
                 input_file_.filename().string());

    std::ifstream input(input_file_);
    // Открываем на дозапись, если файл уже существует (для поддержки цикла в main)
    std::ofstream output(output_file_, std::ios::app);

    if (!input.is_open() || !output.is_open()) {
        spdlog::error("Ошибка открытия файлов");
        return;
    }

    // Если файл новый (размер 0), пишем заголовок.
    // Примечание: В однопоточном режиме main мы сами управляем заголовком,
    // но здесь оставим логику для безопасности.
    // Лучше убрать запись заголовка отсюда в main, но если оставить,
    // нужно проверять позицию.

    // Пропуск заголовка входного файла
    std::string header;
    std::getline(input, header);

    // Инкрементальная медиана (two-heap алгоритм)
    // Важно: эти векторы локальны для вызова функции.
    // Для обработки НЕСКОЛЬКИХ файлов в потоковом режиме нужно передавать состояние,
    // но это усложняет интерфейс. Оставим как есть (обработка одного гигантского файла).
    std::vector<double> max_heap;
    std::vector<double> min_heap;
    double previous_median = -1.0;
    size_t processed_lines = 0;

    stream_buffer buffer(buffer_size_);

    while (buffer.read_chunk(input)) {
        for (const auto& line : buffer.lines()) {
            if (line.empty()) continue;

            // Используем статические методы csv_reader
            auto fields = csv_reader::split(line, ';');
            if (fields.size() < 3) continue;

            auto receive_ts = csv_reader::safe_parse<uint64_t>(fields[0]);
            auto price = csv_reader::safe_parse<double>(fields[2]);

            if (!receive_ts || !price) continue;

            // Добавляем в heap
            if (max_heap.empty() || *price <= max_heap.front()) {
                max_heap.push_back(*price);
                std::push_heap(max_heap.begin(), max_heap.end());
            } else {
                min_heap.push_back(*price);
                std::push_heap(min_heap.begin(), min_heap.end(), std::greater<>{});
            }

            // Балансировка heap
            if (max_heap.size() > min_heap.size() + 1) {
                double val = max_heap.front();
                std::pop_heap(max_heap.begin(), max_heap.end());
                max_heap.pop_back();
                min_heap.push_back(val);
                std::push_heap(min_heap.begin(), min_heap.end(), std::greater<>{});
            } else if (min_heap.size() > max_heap.size() + 1) {
                double val = min_heap.front();
                std::pop_heap(min_heap.begin(), min_heap.end(), std::greater<>{});
                min_heap.pop_back();
                max_heap.push_back(val);
                std::push_heap(max_heap.begin(), max_heap.end());
            }

            // Вычисление медианы
            double current_median = 0.0;
            if (max_heap.size() > min_heap.size()) {
                current_median = max_heap.front();
            } else if (min_heap.size() > max_heap.size()) {
                current_median = min_heap.front();
            } else if (!max_heap.empty() && !min_heap.empty()) {
                current_median = (max_heap.front() + min_heap.front()) / 2.0;
            }

            // Запись при изменении
            if (std::abs(current_median - previous_median) > 1e-10) {
                output << *receive_ts << ";"
                       << std::fixed << std::setprecision(8)
                       << current_median << "\n";
                previous_median = current_median;
            }

            processed_lines++;
        }
    }

    spdlog::info("Потоковая обработка: {} строк обработано", processed_lines);
}

// ... реализация stream_buffer ...
bool streaming_processor::stream_buffer::read_chunk(std::ifstream& file_) {
    if (!file_.good()) return false;

    file_.read(buffer_.data(), buffer_size_);
    std::streamsize bytes_read = file_.gcount();

    if (bytes_read == 0) return false;

    lines_.clear();
    std::string current_line = incomplete_line_;

    for (std::streamsize i = 0; i < bytes_read; ++i) {
        if (buffer_[i] == '\n') {
            lines_.push_back(current_line);
            current_line.clear();
        } else {
            current_line += buffer_[i];
        }
    }

    incomplete_line_ = current_line;
    return true;
}

streaming_processor::stream_buffer::stream_buffer(size_t size_)
    : buffer_size_(size_), buffer_(size_)
{}

} // namespace csv_median_calc
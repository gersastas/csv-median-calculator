/**
 * \file streaming_processor.hpp
 * \brief БОНУС 7.3: Потоковая обработка больших файлов
 */

#pragma once

#include <filesystem>
#include <vector>
#include <string>

namespace csv_median_calc {

/**
 * \brief Процессор для потоковой обработки файлов >RAM
 */
class streaming_processor {
public:
    /**
     * \brief Обрабатывает большой файл потоково
     * \param input_file_ Входной CSV файл
     * \param output_file_ Выходной файл с медианами
     * \param buffer_size_ Размер буфера чтения (по умолчанию 1MB)
     */
    static void process_streaming(
        const std::filesystem::path& input_file_,
        const std::filesystem::path& output_file_,
        size_t buffer_size_ = 1024 * 1024
    );
    
private:
    /**
     * \brief Буфер для потокового чтения
     */
    class stream_buffer {
    public:
        explicit stream_buffer(size_t size_);
        bool read_chunk(std::ifstream& file_);
        const std::vector<std::string>& lines() const { return lines_; }
        
    private:
        size_t buffer_size_;
        std::vector<char> buffer_;
        std::vector<std::string> lines_;
        std::string incomplete_line_;
    };
};

} // namespace csv_median_calc

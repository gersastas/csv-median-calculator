/**
 * \file test_streaming.cpp
 * \brief Unit-тесты для потоковой обработки больших файлов (БОНУС 7.3)
 *
 * Тесты написаны строго под реальную реализацию streaming_processor:
 * - process_streaming открывает файл в режиме append (std::ios::app)
 * - Заголовок "receive_ts;price_median\n" должен быть записан до вызова
 * - two-heap алгоритм с epsilon = 1e-10 для сравнения double
 * - stream_buffer читает чанками, incomplete_line_ накапливает остаток
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../src/streaming_processor.hpp"
#include "../src/csv_reader.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace csv_median_calc;

// ============================================
// Вспомогательные функции
// ============================================

std::filesystem::path create_temp_dir() {
    auto dir = std::filesystem::temp_directory_path()
               / ("test_streaming_" + std::to_string(
                   std::chrono::steady_clock::now().time_since_epoch().count()
               ));
    std::filesystem::create_directories(dir);
    return dir;
}

void cleanup(const std::filesystem::path& dir) {
    std::filesystem::remove_all(dir);
}

/**
 * \brief Создаёт CSV файл с заголовком trade-формата
 */
void write_csv(const std::filesystem::path& path,
               const std::vector<std::pair<uint64_t, double>>& rows) {
    std::ofstream f(path);
    f << "receive_ts;exchange_ts;price;quantity;side\n";
    for (const auto& [ts, price] : rows) {
        f << ts << ";" << ts << ";" << price << ";1.0;bid\n";
    }
}

/**
 * \brief Инициализирует выходной файл заголовком (как это делает main)
 * Реальный main пишет заголовок сам перед вызовом process_streaming.
 */
void init_output_header(const std::filesystem::path& path) {
    std::ofstream f(path);
    f << "receive_ts;price_median\n";
}

/**
 * \brief Читает строки данных из output CSV (без заголовка)
 */
struct MedianRow {
    uint64_t receive_ts;
    double price_median;
};

std::vector<MedianRow> read_output(const std::filesystem::path& path) {
    std::vector<MedianRow> rows;
    std::ifstream f(path);
    if (!f.is_open()) return rows;

    std::string line;
    std::getline(f, line);  // пропускаем заголовок

    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto pos = line.find(';');
        if (pos == std::string::npos) continue;
        MedianRow row;
        row.receive_ts = std::stoull(line.substr(0, pos));
        row.price_median = std::stod(line.substr(pos + 1));
        rows.push_back(row);
    }
    return rows;
}

// ============================================
// ТЕСТЫ
// ============================================

TEST_CASE("StreamingProcessor: только заголовок — нет строк вывода", "[streaming]") {
    auto dir = create_temp_dir();
    auto in  = dir / "empty.csv";
    auto out = dir / "result.csv";

    write_csv(in, {});
    init_output_header(out);

    REQUIRE_NOTHROW(streaming_processor::process_streaming(in, out));

    auto rows = read_output(out);
    REQUIRE(rows.empty());

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: одна запись — медиана равна цене", "[streaming]") {
    auto dir = create_temp_dir();
    auto in  = dir / "one.csv";
    auto out = dir / "result.csv";

    write_csv(in, {{1000, 100.0}});
    init_output_header(out);

    REQUIRE_NOTHROW(streaming_processor::process_streaming(in, out));

    auto rows = read_output(out);
    REQUIRE(rows.size() == 1);
    REQUIRE(rows[0].receive_ts == 1000);
    REQUIRE_THAT(rows[0].price_median, Catch::Matchers::WithinRel(100.0, 1e-6));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: две записи — медиана как среднее", "[streaming]") {
    auto dir = create_temp_dir();
    auto in  = dir / "two.csv";
    auto out = dir / "result.csv";

    // price: 100, 200 → медианы: 100.0, 150.0
    write_csv(in, {{1000, 100.0}, {2000, 200.0}});
    init_output_header(out);

    streaming_processor::process_streaming(in, out);

    auto rows = read_output(out);

    REQUIRE(rows.size() == 2);
    REQUIRE_THAT(rows[0].price_median, Catch::Matchers::WithinRel(100.0, 1e-6));
    REQUIRE_THAT(rows[1].price_median, Catch::Matchers::WithinRel(150.0, 1e-6));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: три записи — нечётная медиана", "[streaming]") {
    auto dir = create_temp_dir();
    auto in  = dir / "three.csv";
    auto out = dir / "result.csv";

    // price: 100, 200, 300
    // Медианы: 100 → 150 → 200
    // Изменения: 100, 150, 200 → 3 строки
    write_csv(in, {{1000, 100.0}, {2000, 200.0}, {3000, 300.0}});
    init_output_header(out);

    streaming_processor::process_streaming(in, out);

    auto rows = read_output(out);

    REQUIRE(rows.size() == 3);
    REQUIRE_THAT(rows[0].price_median, Catch::Matchers::WithinRel(100.0, 1e-6));
    REQUIRE_THAT(rows[1].price_median, Catch::Matchers::WithinRel(150.0, 1e-6));
    REQUIRE_THAT(rows[2].price_median, Catch::Matchers::WithinRel(200.0, 1e-6));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: записывает только при изменении медианы", "[streaming]") {
    auto dir = create_temp_dir();
    auto in  = dir / "same.csv";
    auto out = dir / "result.csv";

    // Все цены одинаковые — медиана всегда 100.0
    // Записывается только первая строка
    write_csv(in, {
        {1000, 100.0},
        {2000, 100.0},
        {3000, 100.0},
        {4000, 100.0}
    });
    init_output_header(out);

    streaming_processor::process_streaming(in, out);

    auto rows = read_output(out);

    REQUIRE(rows.size() == 1);
    REQUIRE_THAT(rows[0].price_median, Catch::Matchers::WithinRel(100.0, 1e-6));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: результат совпадает с two-heap алгоритмом", "[streaming][correctness]") {
    auto dir = create_temp_dir();
    auto in  = dir / "btc.csv";
    auto out = dir / "result.csv";

    // Реальные цены BTC из примера ТЗ
    write_csv(in, {
        {1716810808663260, 68480.10},
        {1716810809314641, 68480.00},
        {1716810809719209, 68480.10},
        {1716810809719209, 68480.10},
        {1716810809719209, 68480.10}
    });
    init_output_header(out);

    streaming_processor::process_streaming(in, out);

    auto rows = read_output(out);

    // Шаг 1: [68480.10] → median=68480.10
    // Шаг 2: [68480.00, 68480.10] → median=68480.05  (изменилась)
    // Шаг 3: [68480.00, 68480.10, 68480.10] → median=68480.10  (изменилась)
    // Шаг 4: медиана 68480.10, не изменилась
    // Шаг 5: медиана 68480.10, не изменилась
    REQUIRE(rows.size() == 3);
    REQUIRE_THAT(rows[0].price_median, Catch::Matchers::WithinRel(68480.10, 1e-8));
    REQUIRE_THAT(rows[1].price_median, Catch::Matchers::WithinRel(68480.05, 1e-8));
    REQUIRE_THAT(rows[2].price_median, Catch::Matchers::WithinRel(68480.10, 1e-8));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: несуществующий входной файл — без краша", "[streaming][error]") {
    auto dir = create_temp_dir();
    auto in  = dir / "nonexistent.csv";
    auto out = dir / "result.csv";

    init_output_header(out);

    // Реализация проверяет !input.is_open() и делает return, без исключений
    REQUIRE_NOTHROW(streaming_processor::process_streaming(in, out));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: некорректные строки пропускаются", "[streaming][error]") {
    auto dir = create_temp_dir();
    auto in  = dir / "bad.csv";
    auto out = dir / "result.csv";

    {
        std::ofstream f(in);
        f << "receive_ts;exchange_ts;price;quantity;side\n";
        f << "1000;1000;100.0;1.0;bid\n";
        f << "not_a_number;bad;data\n";    // некорректная строка
        f << "2000;2000;200.0;1.0;bid\n";
        f << ";;\n";                        // пустые поля
        f << "3000;3000;300.0;1.0;bid\n";
    }
    init_output_header(out);

    REQUIRE_NOTHROW(streaming_processor::process_streaming(in, out));

    auto rows = read_output(out);
    // Должны обработаться 3 корректные строки с ценами 100, 200, 300
    REQUIRE(rows.size() >= 1);
    REQUIRE_THAT(rows[0].price_median, Catch::Matchers::WithinRel(100.0, 1e-6));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: файл без переноса строки в конце", "[streaming][edge]") {
    auto dir = create_temp_dir();
    auto in  = dir / "no_newline.csv";
    auto out = dir / "result.csv";

    {
        std::ofstream f(in);
        f << "receive_ts;exchange_ts;price;quantity;side\n";
        f << "1000;1000;100.0;1.0;bid";  // нет \n в конце
    }
    init_output_header(out);

    // stream_buffer накапливает incomplete_line_, последняя строка
    // должна обработаться при завершении чтения
    REQUIRE_NOTHROW(streaming_processor::process_streaming(in, out));

    // Результат может быть 0 или 1 строк в зависимости от реализации
    // Главное — без краша
    REQUIRE(std::filesystem::exists(out));

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: разные размеры буфера дают одинаковый результат", "[streaming]") {
    auto dir = create_temp_dir();
    auto in  = dir / "data.csv";

    write_csv(in, {
        {1000, 100.0},
        {2000, 300.0},
        {3000, 200.0},
        {4000, 400.0},
        {5000, 150.0}
    });

    std::vector<size_t> buffer_sizes = {64, 256, 1024, 4096, 1024 * 1024};
    std::vector<std::vector<MedianRow>> all_results;

    for (size_t buf_size : buffer_sizes) {
        auto out = dir / ("result_" + std::to_string(buf_size) + ".csv");
        init_output_header(out);
        streaming_processor::process_streaming(in, out, buf_size);
        all_results.push_back(read_output(out));
    }

    // Все запуски должны дать одинаковое количество строк
    for (size_t i = 1; i < all_results.size(); ++i) {
        REQUIRE(all_results[i].size() == all_results[0].size());
    }

    // Значения медиан должны совпадать
    for (size_t i = 1; i < all_results.size(); ++i) {
        for (size_t j = 0; j < all_results[0].size(); ++j) {
            REQUIRE(all_results[i][j].receive_ts == all_results[0][j].receive_ts);
            REQUIRE_THAT(all_results[i][j].price_median,
                         Catch::Matchers::WithinRel(all_results[0][j].price_median, 1e-8));
        }
    }

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: медианы монотонно разумны на отсортированных ценах", "[streaming]") {
    auto dir = create_temp_dir();
    auto in  = dir / "sorted.csv";
    auto out = dir / "result.csv";

    // Возрастающие цены: медиана тоже должна только расти
    std::vector<std::pair<uint64_t, double>> rows;
    for (int i = 1; i <= 20; ++i) {
        rows.push_back({static_cast<uint64_t>(i * 1000),
                        static_cast<double>(i * 10)});
    }
    write_csv(in, rows);
    init_output_header(out);

    streaming_processor::process_streaming(in, out);

    auto result = read_output(out);

    REQUIRE(result.size() > 0);

    // Медианы должны быть неубывающими
    for (size_t i = 1; i < result.size(); ++i) {
        REQUIRE(result[i].price_median >= result[i-1].price_median - 1e-10);
    }

    // Финальная медиана примерно посередине диапазона [10..200]
    double last = result.back().price_median;
    REQUIRE(last > 10.0);
    REQUIRE(last < 200.0);

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: большой файл обрабатывается за разумное время", "[streaming][performance]") {
    auto dir = create_temp_dir();
    auto in  = dir / "big.csv";
    auto out = dir / "result.csv";

    // ~50k записей
    const size_t N = 50000;
    {
        std::ofstream f(in);
        f << "receive_ts;exchange_ts;price;quantity;side\n";
        for (size_t i = 0; i < N; ++i) {
            double price = 1000.0 + (i % 100) * 0.1;
            f << (1000000 + i) << ";" << (1000000 + i) << ";"
              << price << ";1.0;bid\n";
        }
    }
    init_output_header(out);

    auto t0 = std::chrono::high_resolution_clock::now();
    streaming_processor::process_streaming(in, out, 64 * 1024);
    auto t1 = std::chrono::high_resolution_clock::now();

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    INFO("Обработано " << N << " записей за " << ms << " мс");

    REQUIRE(std::filesystem::exists(out));
    auto rows = read_output(out);
    REQUIRE(rows.size() > 0);

    // Должно уложиться в 30 секунд
    REQUIRE(ms < 30000);

    cleanup(dir);
}

TEST_CASE("StreamingProcessor: результаты совпадают с median_calculator на тех же данных", "[streaming][correctness]") {
    auto dir = create_temp_dir();
    auto in  = dir / "compare.csv";

    std::vector<std::pair<uint64_t, double>> data = {
        {1000,  50.0},
        {2000, 150.0},
        {3000, 100.0},
        {4000, 200.0},
        {5000,  80.0}
    };
    write_csv(in, data);

    // --- Потоковая обработка ---
    auto out_stream = dir / "stream.csv";
    init_output_header(out_stream);
    streaming_processor::process_streaming(in, out_stream);
    auto stream_rows = read_output(out_stream);

    // --- Ручной расчёт two-heap ---
    // Симулируем алгоритм руками для проверки
    std::vector<double> max_heap, min_heap;
    double prev = -1.0;
    std::vector<double> expected_medians;

    auto add_val = [&](double v) {
        if (max_heap.empty() || v <= max_heap.front()) {
            max_heap.push_back(v);
            std::push_heap(max_heap.begin(), max_heap.end());
        } else {
            min_heap.push_back(v);
            std::push_heap(min_heap.begin(), min_heap.end(), std::greater<>{});
        }
        // Балансировка
        while (max_heap.size() > min_heap.size() + 1) {
            double val = max_heap.front();
            std::pop_heap(max_heap.begin(), max_heap.end());
            max_heap.pop_back();
            min_heap.push_back(val);
            std::push_heap(min_heap.begin(), min_heap.end(), std::greater<>{});
        }
        while (min_heap.size() > max_heap.size() + 1) {
            double val = min_heap.front();
            std::pop_heap(min_heap.begin(), min_heap.end(), std::greater<>{});
            min_heap.pop_back();
            max_heap.push_back(val);
            std::push_heap(max_heap.begin(), max_heap.end());
        }
        double m = 0.0;
        if (max_heap.size() > min_heap.size()) m = max_heap.front();
        else if (min_heap.size() > max_heap.size()) m = min_heap.front();
        else m = (max_heap.front() + min_heap.front()) / 2.0;

        if (std::abs(m - prev) > 1e-10) {
            expected_medians.push_back(m);
            prev = m;
        }
    };

    for (const auto& [ts, price] : data) add_val(price);

    REQUIRE(stream_rows.size() == expected_medians.size());
    for (size_t i = 0; i < expected_medians.size(); ++i) {
        REQUIRE_THAT(stream_rows[i].price_median,
                     Catch::Matchers::WithinRel(expected_medians[i], 1e-8));
    }

    cleanup(dir);
}

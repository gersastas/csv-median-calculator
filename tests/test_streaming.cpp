/**
 * @file test_streaming.cpp
 * @brief Unit-тесты для модуля streaming_processor (БОНУС 7.3)
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "streaming_processor.hpp"
#include "csv_reader.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>

namespace fs = std::filesystem;
using namespace csv_median_calc;

// Вспомогательная функция для создания временного CSV файла
fs::path create_temp_csv(const std::vector<std::string>& lines) {
    auto temp_dir = fs::temp_directory_path();
    auto temp_file = temp_dir / "test_streaming_XXXXXX.csv";
    // Генерируем уникальное имя (в Linux можно использовать mkstemp, но для простоты добавим timestamp)
    static int counter = 0;
    temp_file = temp_dir / ("test_streaming_" + std::to_string(++counter) + ".csv");

    std::ofstream out(temp_file);
    for (const auto& line : lines) {
        out << line << '\n';
    }
    return temp_file;
}

// Удаление временного файла
void cleanup_temp(const fs::path& path) {
    fs::remove(path);
}

// Чтение выходного файла в вектор пар (receive_ts, median)
std::vector<std::pair<uint64_t, double>> read_output(const fs::path& path) {
    std::vector<std::pair<uint64_t, double>> result;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto fields = csv_reader::split(line, ';');
        if (fields.size() != 2) continue;
        auto ts = csv_reader::safe_parse<uint64_t>(fields[0]);
        auto median = csv_reader::safe_parse<double>(fields[1]);
        if (ts && median) {
            result.emplace_back(*ts, *median);
        }
    }
    return result;
}

TEST_CASE("Streaming processor: empty input file", "[streaming]") {
    auto input = create_temp_csv({}); // пустой файл (даже без заголовка)
    auto output = fs::temp_directory_path() / "stream_out_empty.csv";

    streaming_processor::process_streaming(input, output, 1024);

    REQUIRE(fs::exists(output));
    REQUIRE(fs::file_size(output) == 0); // выходной файл пуст

    cleanup_temp(input);
    fs::remove(output);
}

TEST_CASE("Streaming processor: simple case", "[streaming]") {
    std::vector<std::string> lines = {
        "receive_ts;exchange_ts;price;quantity;side",
        "1000;900;100.0;1.0;bid",
        "2000;1900;102.0;2.0;ask",
        "3000;2900;101.0;1.5;bid",
        "4000;3900;103.0;3.0;ask"
    };
    auto input = create_temp_csv(lines);
    auto output = fs::temp_directory_path() / "stream_out_simple.csv";

    streaming_processor::process_streaming(input, output, 1024);

    auto results = read_output(output);
    REQUIRE(results.size() == 3);
    CHECK(results[0].first == 1000);
    CHECK_THAT(results[0].second, Catch::Matchers::WithinRel(100.0, 1e-10));
    CHECK(results[1].first == 2000);
    CHECK_THAT(results[1].second, Catch::Matchers::WithinRel(101.0, 1e-10));
    CHECK(results[2].first == 4000);
    CHECK_THAT(results[2].second, Catch::Matchers::WithinRel(101.5, 1e-10));

    cleanup_temp(input);
    fs::remove(output);
}

TEST_CASE("Streaming processor: ignore invalid lines", "[streaming]") {
    std::vector<std::string> lines = {
        "receive_ts;exchange_ts;price;quantity;side",
        "1000;900;100.0;1.0;bid",               // валидная
        "2000;1900;abc;2.0;ask",                 // нечисловая цена
        "3000;2900;101.0;1.5;bid",               // валидная
        "4000;3900",                              // слишком мало полей
        "5000;4900;102.0;3.0;ask"                 // валидная
    };
    auto input = create_temp_csv(lines);
    auto output = fs::temp_directory_path() / "stream_out_invalid.csv";

    streaming_processor::process_streaming(input, output, 1024);

    auto results = read_output(output);
    REQUIRE(results.size() == 3);
    CHECK(results[0].first == 1000);
    CHECK_THAT(results[0].second, Catch::Matchers::WithinRel(100.0, 1e-10));
    CHECK(results[1].first == 3000);
    CHECK_THAT(results[1].second, Catch::Matchers::WithinRel(100.5, 1e-10));
    CHECK(results[2].first == 5000);
    CHECK_THAT(results[2].second, Catch::Matchers::WithinRel(101.0, 1e-10));

    cleanup_temp(input);
    fs::remove(output);
}

TEST_CASE("Streaming processor: different buffer sizes", "[streaming]") {
    std::vector<std::string> lines = {
        "receive_ts;exchange_ts;price;quantity;side",
        "1;0;10.0;1.0;bid",
        "2;0;20.0;1.0;bid",
        "3;0;30.0;1.0;bid",
        "4;0;40.0;1.0;bid",
        "5;0;50.0;1.0;bid"
    };
    auto input = create_temp_csv(lines);
    auto output1 = fs::temp_directory_path() / "stream_out_buf1.csv";
    auto output2 = fs::temp_directory_path() / "stream_out_buf2.csv";

    streaming_processor::process_streaming(input, output1, 10);
    streaming_processor::process_streaming(input, output2, 1024);

    auto res1 = read_output(output1);
    auto res2 = read_output(output2);

    REQUIRE(res1 == res2);

    cleanup_temp(input);
    fs::remove(output1);
    fs::remove(output2);
}

TEST_CASE("Streaming processor: many rows", "[streaming][performance]") {
    std::vector<std::string> lines;
    lines.reserve(1001);
    lines.push_back("receive_ts;exchange_ts;price;quantity;side");
    for (int i = 1; i <= 1000; ++i) {
        lines.push_back(std::to_string(i*1000) + ";0;" + std::to_string(i*1.0) + ";1.0;bid");
    }
    auto input = create_temp_csv(lines);
    auto output = fs::temp_directory_path() / "stream_out_many.csv";

    streaming_processor::process_streaming(input, output, 4096);

    auto results = read_output(output);
    REQUIRE(results.size() > 0);
    CHECK_THAT(results.back().second, Catch::Matchers::WithinRel(500.5, 1e-10));

    cleanup_temp(input);
    fs::remove(output);
}

TEST_CASE("Streaming processor: appending to existing output", "[streaming]") {
    auto input1 = create_temp_csv({
        "receive_ts;exchange_ts;price;quantity;side",
        "100;0;10.0;1.0;bid"
    });
    auto input2 = create_temp_csv({
        "receive_ts;exchange_ts;price;quantity;side",
        "200;0;20.0;1.0;bid"
    });
    auto output = fs::temp_directory_path() / "stream_out_append.csv";

    streaming_processor::process_streaming(input1, output, 1024);
    auto res1 = read_output(output);
    REQUIRE(res1.size() == 1);
    CHECK(res1[0].first == 100);
    CHECK_THAT(res1[0].second, Catch::Matchers::WithinRel(10.0, 1e-10));

    streaming_processor::process_streaming(input2, output, 1024);
    auto res2 = read_output(output);
    REQUIRE(res2.size() == 2);
    CHECK(res2[0].first == 100);
    CHECK(res2[1].first == 200);
    CHECK_THAT(res2[1].second, Catch::Matchers::WithinRel(20.0, 1e-10));

    cleanup_temp(input1);
    cleanup_temp(input2);
    fs::remove(output);
}
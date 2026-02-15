#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include "../src/csv_reader.hpp"

using namespace csv_median_calc;

TEST_CASE("csv_reader читает корректный файл", "[csv]") {
    auto temp = std::filesystem::temp_directory_path() / "test.csv";

    std::ofstream file(temp);
    file << "receive_ts;exchange_ts;price;quantity;side\n";
    file << "1000;900;100.0;1.0;bid\n";
    file << "2000;1900;200.0;2.0;ask\n";
    file.close();

    auto records = csv_reader::read_and_merge({temp});

    REQUIRE(records.size() == 2);
    REQUIRE(records[0].price == 100.0);
    REQUIRE(records[1].price == 200.0);

    std::filesystem::remove(temp);
}


TEST_CASE("csv_reader пропускает некорректные строки", "[csv]") {
    auto temp = std::filesystem::temp_directory_path() / "test_bad.csv";

    std::ofstream file(temp);
    file << "receive_ts;exchange_ts;price;quantity;side\n";
    file << "1000;900;100.0;1.0;bid\n";
    file << "BAD_LINE\n";
    file << "2000;1900;-50.0;2.0;ask\n";
    file.close();

    auto records = csv_reader::read_and_merge({temp}); // ← ВАЖНО

    REQUIRE(records.size() == 1);
    REQUIRE(records[0].price == 100.0);

    std::filesystem::remove(temp);
}

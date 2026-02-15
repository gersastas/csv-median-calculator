#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <filesystem>

#include "../src/median_calculator.hpp"

using namespace csv_median_calc;

TEST_CASE("save_results корректно записывает CSV", "[median][io]") {
    std::vector<median_result> results = {
        {1000, 100.0},
        {2000, 150.0}
    };

    auto temp_file = std::filesystem::temp_directory_path() / "median_test.csv";

    REQUIRE(median_calculator::save_results(results, temp_file));

    std::ifstream file(temp_file);
    REQUIRE(file.is_open());

    std::string header;
    std::getline(file, header);
    REQUIRE(header == "receive_ts;price_median");

    std::string line;
    std::getline(file, line);
    REQUIRE(line.find("1000") != std::string::npos);

    file.close();
    std::filesystem::remove(temp_file);
}

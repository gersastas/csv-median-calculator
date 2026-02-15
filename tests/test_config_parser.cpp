#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <filesystem>

#include "../src/config_parser.hpp"

using namespace csv_median_calc;

TEST_CASE("config_parser парсит валидный TOML", "[config]") {
    auto temp = std::filesystem::temp_directory_path() / "valid_config.toml";

    std::ofstream file(temp);
    file << "[main]\n"
         << "input = '/tmp'\n"
         << "output = '/tmp'\n"
         << "filename_mask = ['trade', 'level']\n";
    file.close();

    auto cfg = config_parser::parse(temp);

    REQUIRE(cfg.input_dir == "/tmp");
    REQUIRE(cfg.output_dir == "/tmp");
    REQUIRE(cfg.filename_masks.size() == 2);

    std::filesystem::remove(temp);
}

TEST_CASE("config_parser выбрасывает при отсутствии input", "[config][edge]") {
    auto temp = std::filesystem::temp_directory_path() / "bad_config.toml";

    std::ofstream file(temp);
    file << "[main]\noutput = '/tmp'\n";
    file.close();

    REQUIRE_THROWS(config_parser::parse(temp));

    std::filesystem::remove(temp);
}

TEST_CASE("config_parser выбрасывает при отсутствии файла", "[config][edge]") {
    REQUIRE_THROWS(config_parser::parse("nonexistent.toml"));
}

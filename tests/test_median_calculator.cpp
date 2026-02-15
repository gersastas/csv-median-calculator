#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "../src/median_calculator.hpp"
#include "../src/csv_reader.hpp"

using namespace csv_median_calc;

TEST_CASE("median_calculator базовый сценарий", "[median]") {
    std::vector<price_record> records = {
        {1000, 900, 100.0, 1.0, "bid", 0},
        {2000, 1900, 200.0, 2.0, "ask", 0},
        {3000, 2900, 300.0, 1.5, "bid", 0}
    };

    auto results = median_calculator::calculate(records);

    REQUIRE(results.size() == 3);
    REQUIRE_THAT(results[0]._median, Catch::Matchers::WithinRel(100.0, 0.0001));
    REQUIRE_THAT(results[1]._median, Catch::Matchers::WithinRel(150.0, 0.0001));
    REQUIRE_THAT(results[2]._median, Catch::Matchers::WithinRel(200.0, 0.0001));
}

TEST_CASE("медиана записывается только при изменении", "[median][edge]") {
    std::vector<price_record> records = {
        {1,0,100,0,"bid",0},
        {2,0,100,0,"bid",0},
        {3,0,100,0,"bid",0}
    };

    auto results = median_calculator::calculate(records);

    REQUIRE(results.size() == 1);
    REQUIRE(results[0]._median == 100.0);
}

TEST_CASE("четное количество элементов", "[median][edge]") {
    std::vector<price_record> records = {
        {1,0,100,0,"bid",0},
        {2,0,200,0,"bid",0}
    };

    auto results = median_calculator::calculate(records);

    REQUIRE(results.size() == 2);
    REQUIRE(results[1]._median == 150.0);
}

TEST_CASE("отрицательные значения", "[median][edge]") {
    std::vector<price_record> records = {
        {1,0,-10,0,"bid",0},
        {2,0, 10,0,"bid",0},
        {3,0, 20,0,"bid",0}
    };

    auto results = median_calculator::calculate(records);

    REQUIRE(results.back()._median == 10.0);
}

TEST_CASE("stress test 1000 элементов", "[median][stress]") {
    std::vector<price_record> records;

    for (int i = 1; i <= 1000; ++i) {
        records.push_back({static_cast<uint64_t>(i),static_cast<uint64_t>(0),
                                                    double(i),
                                                    0.0,
                                                    "bid",
                                                    0
        });

    }

    auto results = median_calculator::calculate(records);

    REQUIRE(results.back()._median == 500.5);
}

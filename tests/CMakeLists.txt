add_executable(unit_tests
        test_main.cpp
        test_median_calculator.cpp
        test_config_parser.cpp
        test_csv_reader.cpp
)

target_link_libraries(unit_tests PRIVATE
        core
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(unit_tests)

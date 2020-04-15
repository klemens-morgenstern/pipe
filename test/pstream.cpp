#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <pstream.hpp>

TEST_CASE("pstream self test", "[]")
{
    process::pstream pstr;

    std::string test_string = "1234567890";
    pstr.write(test_string.data(), test_string.size());
    pstr.flush();

    std::string buffer;
    buffer.resize(test_string.size());
    pstr.read(buffer.data(), buffer.size());

    REQUIRE(buffer == test_string);
}

TEST_CASE("pstream split test", "[]")
{
    auto [r, w] = process::pstream();

    std::string test_string = "1234567890";
    w.write(test_string.data(), test_string.size());
    w.flush();

    std::string buffer;
    buffer.resize(test_string.size());
    r.read(buffer.data(), buffer.size());

    REQUIRE(buffer == test_string);
}
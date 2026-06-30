#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("variables") {
    TEST_CASE("basic @set and substitution") {
        fd::sess::SESS sheet;
        sheet.load_from_string(R"(
            @set accent = #89b4faff;
            @set size = 42;

            button {
                background-color: @accent;
                width: @size;
            }
        )");

        auto s = sheet["button"];
        CHECK(s.color("background-color").to_rgba() == 0x89b4faff);
        CHECK(s.number("width") == 42);
    }
}
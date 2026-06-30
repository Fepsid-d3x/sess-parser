#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("resolve") {
    fd::sess::SESS sheet;

    TEST_CASE("cascade with state") {
        sheet.load_from_string(R"(
            button {
                background-color: #313244;
                padding: 8;
            }
            button:hover {
                background-color: #45475a;
            }
            .large {
                height: 48;
            }
        )");

        auto s = sheet.resolve("button", "", {".large"}, fd::sess::SESSState::Hover);

        CHECK(s.color("background-color").to_rgba() == 0x45475aff);
        CHECK(s.number("height") == 48);
        CHECK(s.number("padding") == 8);
    }
}
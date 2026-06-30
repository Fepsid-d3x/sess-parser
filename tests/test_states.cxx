#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("states") {
    fd::sess::SESS sheet;

    TEST_CASE("state selectors") {
        sheet.load_from_string(R"(
            button {
                background-color: #313244;
            }
            button:hover {
                background-color: #45475a;
            }
            button:active {
                background-color: #111;
            }
        )");

        auto normal = sheet["button"];
        auto hover = sheet["button:hover"];

        CHECK(normal.color("background-color").to_rgba() == 0x313244ff);
        CHECK(hover.color("background-color").to_rgba() == 0x45475aff);
    }

    TEST_CASE("resolve with combined states") {
        auto s = sheet.resolve("button", "", {}, 
            fd::sess::SESSState::Hover | fd::sess::SESSState::Focused);
    }
}
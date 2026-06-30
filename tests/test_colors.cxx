#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("colors") {
    fd::sess::SESS sheet;

    TEST_CASE("hex colors") {
        sheet.load_from_string(R"(
            item {
                c1: #ff0000;
                c2: #00ff00ff;
                c3: #abc;
            }
        )");

        auto s = sheet["item"];
        CHECK(s.color("c1").to_rgba() == 0xff0000ff);
        CHECK(s.color("c2").to_rgba() == 0x00ff00ff);
        CHECK(s.color("c3").to_rgba() == 0xaabbccff);
    }

    TEST_CASE("named colors") {
        sheet.load_from_string("item { red: red; transparent: transparent; }");
        auto s = sheet["item"];
        CHECK(s.color("red").r == 255);
        CHECK(s.color("transparent").a == 0);
    }
}
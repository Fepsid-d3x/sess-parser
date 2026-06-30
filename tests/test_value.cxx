#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("value") {
    fd::sess::SESS sheet;

    TEST_CASE("type conversion and fallbacks") {
        sheet.load_from_string(R"(
            item {
                num: 42;
                flag: true;
                text: hello;
                missing: something;
            }
        )");

        auto s = sheet["item"];

        CHECK(s.number("num") == 42);
        CHECK(s.boolean("flag") == true);
        CHECK(s.string("text") == "hello");

        CHECK(s.number("missing", 999) == 999);
        CHECK(s.boolean("missing", false) == false);
        CHECK(s.color("missing", {255, 0, 0}).r == 255);
    }
}
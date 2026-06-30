#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("anchor & pivot") {
    fd::sess::SESS sheet;

    TEST_CASE("anchor parsing") {
        sheet.load_from_string(R"(
            widget {
                anchor: center;
                pivot: top-left;
            }
            widget2 {
                anchor: bottom-right;
            }
        )");

        auto s1 = sheet["widget"];
        auto s2 = sheet["widget2"];

        CHECK(s1.anchor("anchor") == fd::sess::UIAnchor::Center);
        CHECK(s1.pivot("pivot") == fd::sess::UIPivot::TopLeft);
        CHECK(s2.anchor("anchor") == fd::sess::UIAnchor::BottomRight);
    }

    TEST_CASE("default values") {
        sheet.load_from_string("widget {}");
        auto s = sheet["widget"];

        CHECK(s.anchor("anchor", fd::sess::UIAnchor::TopLeft) == fd::sess::UIAnchor::TopLeft);
    }
}
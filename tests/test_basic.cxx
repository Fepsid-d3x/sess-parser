#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("basic") {
    TEST_CASE("load from string") {
        fd::sess::SESS sheet;
        bool ok = sheet.load_from_string("button { width: 100; }");
        CHECK(ok);
        CHECK(sheet.ok());
    }

    TEST_CASE("non-existent file returns false") {
        fd::sess::SESS sheet;
        bool ok = sheet.load("file.sess");
        CHECK_FALSE(ok);
    }
}
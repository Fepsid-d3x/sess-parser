#include "doctest.h"
#include "../sess.hpp"

TEST_SUITE("include") {
    TEST_CASE("simple @include") {
        fd::sess::SESS sheet;
        
        bool ok = sheet.load_from_string(R"(
            @include "theme/colors.sess"
            
            button {
                background-color: @accent;
            }
        )");
        
        CHECK(ok); 
    }
}
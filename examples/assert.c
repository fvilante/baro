// Keep assert around, even in release builds, for the sake of tests
#undef NDEBUG

#include <baro.h>

// Try using the assert macro provided by baro.h, which turns into a regular
// BARO_REQUIRE
TEST("overridden assert") {
    assert(1 == 1);
    assert(1 == 2);
}

#include <assert.h>

// Try using the assert macro provided by assert.h, which will throw a SIGABRT
TEST("libc assert") {
    assert(1 == 1);
    assert(1 == 2);
}

// Make sure tests after the SIGABRT still work
TEST("some other test") {
    REQUIRE(1 == 2);
}
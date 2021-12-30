#include <baro.h>

TEST("basic") {
    CHECK(1);
    CHECK(0);

    CHECK_FALSE(0);
    CHECK_FALSE(1);

    CHECK_EQ(0, 0);
    CHECK_EQ(0, 1);
    CHECK_EQ(1, 0);
    CHECK_EQ(1, 1);

    CHECK_NE(0, 0);
    CHECK_NE(0, 1);
    CHECK_NE(1, 0);
    CHECK_NE(1, 1);

    CHECK_LT(0, 0);
    CHECK_LT(0, 1);
    CHECK_LT(1, 0);
    CHECK_LT(1, 1);

    CHECK_LE(0, 0);
    CHECK_LE(0, 1);
    CHECK_LE(1, 0);
    CHECK_LE(1, 1);

    CHECK_GE(0, 0);
    CHECK_GE(0, 1);
    CHECK_GE(1, 0);
    CHECK_GE(1, 1);

    CHECK_GT(0, 0);
    CHECK_GT(0, 1);
    CHECK_GT(1, 0);
    CHECK_GT(1, 1);
}

TEST("require") {
    REQUIRE(1);

    REQUIRE(0);
}

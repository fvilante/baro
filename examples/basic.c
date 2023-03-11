#include <baro.h>

TEST("check fails soft") {
    CHECK(1);
    CHECK(0); // should fail

    CHECK_FALSE(0);
    CHECK_FALSE(1); // should fail

    CHECK_EQ(0, 0);
    CHECK_EQ(0, 1); // should fail
    CHECK_EQ(1, 0); // should fail
    CHECK_EQ(1, 1);

    CHECK_NE(0, 0); // should fail
    CHECK_NE(0, 1);
    CHECK_NE(1, 0);
    CHECK_NE(1, 1); // should fail

    CHECK_LT(0, 0); // should fail
    CHECK_LT(0, 1);
    CHECK_LT(1, 0); // should fail
    CHECK_LT(1, 1); // should fail

    CHECK_LE(0, 0);
    CHECK_LE(0, 1);
    CHECK_LE(1, 0); // should fail
    CHECK_LE(1, 1);

    CHECK_GE(0, 0);
    CHECK_GE(0, 1); // should fail
    CHECK_GE(1, 0);
    CHECK_GE(1, 1);

    CHECK_GT(0, 0); // should fail
    CHECK_GT(0, 1); // should fail
    CHECK_GT(1, 0);
    CHECK_GT(1, 1); // should fail
}

TEST("require fails hard") {
    REQUIRE(1);

    REQUIRE(0); // should fail

    REQUIRE(0);
}

TEST("string comparisons") {
    char const *foo = "foo";
    CHECK_STR_EQ("foo", foo);
    CHECK_STR_NE("foo", foo); // should fail

    char *foo_dup = strdup(foo);
    CHECK_STR_EQ(foo, foo_dup);
    CHECK_STR_EQ("foo", foo_dup);

    CHECK_STR_ICASE_EQ("Foo", foo);
    CHECK_STR_ICASE_EQ("FOO", foo_dup);
    CHECK_STR_ICASE_EQ("BAR", foo_dup); // should fail

    free(foo_dup);
}

TEST("fail with message") {
    CHECK(0, "displays message on check failure");
    CHECK(1, "doesn't display message on check pass");

    REQUIRE(1, "doesn't display message on require pass");
    REQUIRE(0, "displays message on require failure");
}

TEST("string comparisons with messages") {
    CHECK_STR_EQ("foo", "bar", "message");
    CHECK_STR_NE("foo", "foo", "message");
    CHECK_STR_ICASE_EQ("foo", "bar", "message");
    CHECK_STR_ICASE_NE("foo", "foo", "message");
}
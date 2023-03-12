#include <baro.h>

// Assuming the suite is executed with "-t foo,bar":

TEST("[foo] This test will be ran") {
    REQUIRE(0);
}

TEST("[bar] This test will also be ran") {
    REQUIRE(0);
}

TEST("[baz] [foo] And this one...") {
    REQUIRE(0);
}

TEST("[baz] But not this one.") {
    REQUIRE(0);
}

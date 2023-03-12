#include <baro.h>

// Assuming 5 partitions:

// Partition 1
// ./example_partitioning -n 1 -p 5

TEST("1") { REQUIRE(0); }
TEST("2") { REQUIRE(0); }

// Partition 2
// ./example_partitioning -n 2 -p 5

TEST("3") { REQUIRE(0); }
TEST("4") { REQUIRE(0); }

// Partition 3
// ./example_partitioning -n 3 -p 5

TEST("5") { REQUIRE(0); }
TEST("6") { REQUIRE(0); }

// Partition 4
// ./example_partitioning -n 4 -p 5

TEST("7") { REQUIRE(0); }
TEST("8") { REQUIRE(0); }

// Partition 5
// ./example_partitioning -n 5 -p 5

TEST("9") { REQUIRE(0); }
TEST("10") { REQUIRE(0); }

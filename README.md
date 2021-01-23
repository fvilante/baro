# `baro`

![Tests](https://github.com/branw/baro/workflows/CMake/badge.svg)

A C99 unit testing framework that is:

- **modern** -- write your tests alongside your implementation
- **ultra-light** -- just include a 1k LOC header file
- **portable** -- designed for Windows and *nix

## Usage

In your source files, include `baro.h` and begin writing tests:

```c
#include <baro.h>

magic_t *magic_create(void) { /* ... */ }

void magic_destroy(magic_t *magic) { /* ... */ }

int magic_frombobulate(magic_t *magic, elixir_t *elixir) { /* ... */ }

// Write tests directly after function definitions, or where
// ever you want, really
TEST("[magic] frombobulation") {
    magic_t *magic = magic_create();
  
    // Subtests allow you to write several independent test cases; this
    // is very useful for testing anything with common setup & teardown
    SUBTEST("with elixir") {
        elixir_t *panacea = elixir_create_panacea();
          
        // Require will assert that the returned value is greater than or equal
        // to 100, and fail the (sub)test if not
        REQUIRE_GE(magic_frombobulate(magic, panacea), 100);
        
        SUBTEST("weaker on the second application") {
            // Check is less strict that require and will let the test case
            // continue, despite being marked as failing
            CHECK_LT(magic_frombobulate(magic, panacea), 50);
        }
    }
    
    magic_destroy(magic);
}

char *magic_name(magic_t const *magic) { /* ... */ }

TEST("[magic] name") {
    magic_t *magic = magic_create();
    
    REQUIRE_STR_ICASE_EQ(magic_name(magic), "default");
    
    SUBTEST("after frombobulating") {
        elixir_t *mana = elixir_create_mana();
        
        REQUIRE_EQ(magic_frombobulate(magic, mana), 12, "frombobulate must be 12");
        CHECK_STR_ICASE_EQ(magic_name(magic), "strong");
    }
    
    magic_destroy(magic);
}}
```

Then, build a testing executable with your regular source files and a single
file containing a `BARO_MAIN` definition (such as the provided `baro.c`):

```c
#define BARO_MAIN
#include <baro.h>
```

Also, make sure to add a `BARO_ENABLE` definition to all files including
`baro.h`, otherwise the tests will not be generated. Run the binary and viola:

```plain
> ./magic-tests
Running 10 out of 10 tests
============================================================
Check less than failed:
    magic_frombobulate(magic, panacea) < 50
==> 62 < 50
At magic.c:25
  In: [magic] frombobulation (magic.c:11)
    Under: with elixir (magic.c:16)
      Under: weaker on the second application (magic.c:23)
============================================================
tests:      10 total |     9 passed |     1 failed
asserts:    18 total |    17 passed |     1 failed
```

### Macros

`baro`'s interface is exposed entirely through macros. By default, these macros
are in short form, however a prefix can be required by defining
`BARO_NO_SHORT`.

#### Assertions

Assertions begin with either `CHECK` or `REQUIRE`. `REQUIRE` will stop
execution and fail a test immediately on failure. `CHECK`, however, will
mark a test as failed but continue executing it.

|`baro.h` code|`assert.h` equivalent|
|----|----------|
|`CHECK(x)`|`assert(x)`|
|`CHECK_FALSE(x)`|`assert(!x)`|
|`CHECK_EQ(a, b)`|`assert(a == b)`|
|`CHECK_NE(a, b)`|`assert(a != b)`|
|`CHECK_LT(a, b)`|`assert(a < b)`|
|`CHECK_LE(a, b)`|`assert(a <= b)`|
|`CHECK_GT(a, b)`|`assert(a > b)`|
|`CHECK_GE(a, b)`|`assert(a >= b)`|
|`CHECK_STR_EQ(a, b)`|`assert(!strcmp(a, b))`|
|`CHECK_STR_NE(a, b)`|`assert(strcmp(a, b) != 0)`|
|`CHECK_STR_ICASE_EQ(a, b)`|`assert(!strcmpi(a, b))`|
|`CHECK_STR_ICASE_NE(a, b)`|`assert(strcmpi(a, b) != 0)`|

A message can also be provided with any assertion as a last parameter, e.g.:
```c
REQUIRE(file_exists("foo.txt"), "need test data file");
CHECK_STR_EQ(get_name(), "testuser", "name should be populated");
```

#### Subtests

Subtests allow for test cases to be arranged in a tree: the outermost `TEST`
scope is the root, and each `SUBTEST` creates a child node. A test with
multiple subtests will be executed once for every leaf node, meaning that
`SUBTEST` branches at the same level can work independently. For example:

<table><tr>
<td>Code</td>
<td>Output</td>
</tr><tr>
<td><pre>TEST("i like subtests") {
    printf("begin\n");
    SUBTEST("1") {
        printf("1\n");
        SUBTEST("1.1") {
            printf("1.1\n");
        }
    }
    SUBTEST("2") {
        printf("2\n");
        SUBTEST("2.1") {
            printf("2.1\n");
        }
        SUBTEST("2.2") {
            printf("2.2\n");
            SUBTEST("2.2.1") {
                printf("2.2.1\n");
                SUBTEST("2.2.1.1") {
                    printf("2.2.1.1\n");
                }
                SUBTEST("2.2.1.2") {
                    printf("2.2.1.2\n");
                }
            }
        }
        SUBTEST("2.3") {
            printf("2.3\n");
        }
        SUBTEST("2.4") {
            printf("2.4\n");
        }
        printf("\n");
    }
}</pre></td>
<td><pre>begin
1
1.1

begin
2
2.1

begin
2
2.2
2.2.1
2.2.1.1

begin
2
2.2
2.2.1
2.2.1.2

begin
2
2.3

begin
2
2.4</pre></td>
</tr></table>

### Command-line arguments

The test runner accepts a few arguments:

- `-a` shows **a**ll tests, even passing ones
- `-o` shows all standard **o**utput, even for passing tests
  - By default, [only the last 4096 characters of] standard output will be
    shown only for failing tests
- `-s` will cause the test suite to **s**top after the first failure

#### Partitioning

By default, all test cases are ran in a single thread. You can speed up
execution and parallelize by creating multiple processes with the partitioning
arguments:

- `-n <current_partition>` sets the current partition index (0-based)
- `-p <partition_count>` sets the total number of partitions to make (1-based)

For example, if we have 100 tests and want to partition across five processes:
```bash
./tests -n 0 -p 5 # runs tests 0-19 inclusive 
./tests -n 1 -p 5 # " 20-39
./tests -n 2 -p 5 # " 40-59
./tests -n 3 -p 5 # " 60-79
./tests -n 4 -p 5 # " 80-99
```

Or, with GNU Parallel:

```bash
parallel ./tests -n {} -p 5 ::: {0..4}
```

## License

`baro` is released under the MIT License. See `LICENSE` for more info.

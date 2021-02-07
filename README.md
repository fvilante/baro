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
`baro.h`, otherwise the tests will not be generated:
- Directly via `gcc`/`clang`, you can add `-DBARO_ENABLE`:
  ```
  gcc source.c -DBARO_ENABLE
  ```
- With `make`, you can add it to your `CFLAGS`:
  ```plain
  CFLAGS += -DBARO_ENABLE

  target: source.c
    $(CC) $(CFLAGS) -o target $^
  ```
- With `cmake`, you can add it as a compile definition to a target:
  ```cmake
  add_executable(test-my-stuff ext/baro/baro.c ${MY_SOURCE_FILES})
  target_compile_definitions(test-my-stuff PRIVATE BARO_ENABLE)
  ```

Run the binary and viola:

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
are in short form, however a `BARO_` prefix can be required by defining
`BARO_NO_SHORT`.

#### Assertions

Assertions begin with either `CHECK` or `REQUIRE`. Whereas `REQUIRE` will stop
the execution of a test after marking it as failed, `CHECK` will let the test
continue running.

|`baro.h` code|`assert.h` equivalent|
|----|----------|
|`REQUIRE(x)`|`assert(x)`|
|`REQUIRE_FALSE(x)`|`assert(!x)`|
|`REQUIRE_EQ(a, b)`|`assert(a == b)`|
|`REQUIRE_NE(a, b)`|`assert(a != b)`|
|`REQUIRE_LT(a, b)`|`assert(a < b)`|
|`REQUIRE_LE(a, b)`|`assert(a <= b)`|
|`REQUIRE_GT(a, b)`|`assert(a > b)`|
|`REQUIRE_GE(a, b)`|`assert(a >= b)`|
|`REQUIRE_STR_EQ(a, b)`|`assert(!strcmp(a, b))`|
|`REQUIRE_STR_NE(a, b)`|`assert(strcmp(a, b) != 0)`|
|`REQUIRE_STR_ICASE_EQ(a, b)`|`assert(!strcmpi(a, b))`|
|`REQUIRE_STR_ICASE_NE(a, b)`|`assert(strcmpi(a, b) != 0)`|

Note that both `REQUIRE(a < b)` is functionally equivalent to
`REQUIRE_LT(a, b)`, however the latter will provide more information for
investigating failures:

<table><thead><tr>
<th><code>REQUIRE(a < b)</code></th>
<th><code>REQUIRE_LT(a, b)</code></th>
</tr></thead><tbody><tr>
<td><pre>Require failed:
    a < b != 0
==> 0 != 0</pre></td>
<td><pre>Require failed:
    a < b
==> 2 < 1</pre></td>
</tr></tbody></table>

A descriptive message can be provided to display on failure by passing in
another parameter to any assertion:

```c
REQUIRE(file_exists("foo.txt"), "need test data file");
CHECK_STR_EQ(get_name(), "testuser", "name should be populated");
```

#### Subtests

Subtests allow for test cases to be arranged in a tree: the outermost `TEST`
scope is the root, and each `SUBTEST` creates a child node. A test with
multiple subtests will be executed once for every leaf node, meaning that
`SUBTEST` branches at the same level can work independently. For example:

<table><thead><tr>
<th>Code</th>
<th>Output</th>
</tr></thead><tbody><tr>
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
    }
    printf("\n");
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
</tr></tbody></table>

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

- `-n <current_partition>` sets the current partition index (1-based)
- `-p <partition_count>` sets the total number of partitions to make (1-based)

For example, if we have 100 tests and want to partition across five processes:
```bash
./tests -n 1 -p 5 # runs tests 0-19 inclusive
./tests -n 2 -p 5 # " 20-39
./tests -n 3 -p 5 # " 40-59
./tests -n 4 -p 5 # " 60-79
./tests -n 5 -p 5 # " 80-99
```

Or, with GNU Parallel:

```bash
parallel ./tests -n {} -p 5 ::: {1..5}
```

## License

`baro` is released under the MIT License. See `LICENSE` for more info.

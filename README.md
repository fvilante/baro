# `baro`

![CMake Build Status Badge](https://github.com/branw/baro/workflows/CMake/badge.svg)

A C99 unit testing framework that is:

- **modern** -- write your tests alongside your implementation
- **ultra-light** -- just include a 1k LOC header file
- **portable** -- designed for Windows and *nix

## Usage

1. Include `baro.h` in your source files and start writing tests:
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

2. Create a new build target with all the same source files, except the file
   containing the `main` function. Instead, use the provided `baro.c`.
   - With CMake:
     ```cmake
     # add_executable(app app.c main.c)
     
     add_executable(tests app.c ext/baro/baro.c)
     
     enable_testing()
     add_test(app tests)
     ```

3. Define `BARO_ENABLED` for this new build target only (and _not_ for any 
   non-test targets).
   - With `gcc`/`clang`, pass `-DBARO_ENABLE`:
     ```plain
     gcc source.c baro.c -o tests -DBARO_ENABLE
     ```

   - With `cmake`, add it as a compile definition:
     ```cmake
     target_compile_definitions(tests PRIVATE BARO_ENABLE)
     ```

5. Build and run the new build target:
    ```plain
    > ./tests
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

`baro` is a small set of macros. By default, both a long (`BARO_`-prefixed) and
short (un-prefixed) form are defined. The short form can be disabled by defining
`BARO_NO_SHORT`.

#### Assertions

Assertions begin with either `CHECK` or `REQUIRE`. `CHECK` will fail quietly,
allowing the proceeding statements to run, while `REQUIRE` will fail hard and
cause the current test to end immediately.

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

Note that `REQUIRE(a < b)` is functionally equivalent to `REQUIRE_LT(a, b)`.
The more specific set of functions will provide a bit more context to failures
however:

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

All these macros also accept a description as an additional parameter:

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
  - By default, only the last 4096 characters of standard output will be
    shown only for failing tests
- `-s` will cause the test suite to **s**top after the first failure

#### Partitioning

By default, all test cases are executed in a single thread. You can speed up
execution by creating multiple processes with the partitioning arguments:

- `-n <current_partition>` sets the current partition index (1-based)
- `-p <partition_count>` sets the total number of partitions to make (1-based)

For example, if we have 100 tests and want to partition across five processes:
```bash
./tests -n 1 -p 5 # runs tests 0-19 (inclusive)
./tests -n 2 -p 5 # "          20-39
./tests -n 3 -p 5 # "          40-59
./tests -n 4 -p 5 # "          60-79
./tests -n 5 -p 5 # "          80-99
```

Or, with GNU Parallel:

```bash
parallel ./tests -n {} -p 5 ::: {1..5}
```

## License

`baro` is released under the MIT License. See `LICENSE` for more info.

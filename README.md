# `baro`

A C99 unit testing framework that is:

- **ultra-light**
- **portable**

## Usage

Include `baro.h` and write your tests alongside your implementation:
 
```c
magic_t magic_create(void) { /* ... */ }


#include <baro.h>

TEST("Magic") {
  magic_t magic = magic_create();
 
  SUBTEST("") {
    
  }
}
```

Then, build a testing executable with your regular source files and a single
file containing a `BARO_MAIN` definition (such as the provided `baro.c`):

```c
#define BARO_MAIN
#include <baro.h>
```

Also, make sure to add a `BARO_ENABLE` definition to all files including
`baro.h`, otherwise the tests will not be generated.

## License

`baro` is released under the MIT License. See `LICENSE` for more info.

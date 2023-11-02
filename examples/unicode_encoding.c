#include <baro.h>

// Write your methods like usual:

unsigned long utf8_decode(char **s) {
    uint32_t const f = ~((uint32_t)**(unsigned char**)s << 24u);
#ifdef _WIN32
    int k = 0;
    if (_BitScanReverse(&k, f)) {
        k = 31 - k;
    }
    else {
        k = 32;
    }
#else
    int k = **s ? __builtin_clz(f) : 0;
#endif
    unsigned long mask = (unsigned) (1u << (8u - k)) - 1;
    unsigned long value = **(unsigned char **)s & mask;
    for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {
        value <<= 6u;
        value += (**(unsigned char **)s & 0x3Fu);
    }
    return value;
}

size_t utf8_encode(unsigned long code_point, char **buf, size_t len) {
    char encoded[4] = {0};
    unsigned long lead_byte_max = 0x7f, encoded_len = 0;

    while (code_point > lead_byte_max) {
        encoded[encoded_len++] = (char) ((code_point & 0x3fu) | 0x80u);
        code_point >>= 6u;
        lead_byte_max >>= (encoded_len == 1 ? 2u : 1u);
    }

    encoded[encoded_len++] = (char) ((code_point & lead_byte_max) | (~lead_byte_max << 1u));

    size_t index = encoded_len, written = 0;
    while (index-- && written++ < len) {
        *(*buf)++ = encoded[index];
    }
    return written;
}

// Then, write your tests in the same file (or wherever, really):

TEST("[encoding] UTF-8 <-> UTF-32") {
    // Perform some setup that will be executed for each subtest
    struct test_case {
        char const *utf8;
        uint32_t utf32[32];
    } const
            ascii = {"abCD12!@", {'a', 'b', 'C', 'D', '1', '2', '!', '@', 0}},
            greek = {"κόσμε", {954, 972, 963, 956, 949, 0}},
            emoji = {"\xF0\x9F\x98\x8E\xF0\x9F\x98\xB8", {128526, 128568, 0}},
            *test_cases[] = {
            &ascii, &greek, &emoji,
    };

    size_t const num_test_cases = sizeof(test_cases)/sizeof(struct test_case *);

    // Require that the two values are equal, failing with a message if not.
    REQUIRE_EQ(num_test_cases, 3, "unexpected number of test cases");

    // Branch out into a subtest
    SUBTEST("encode UTF-32 to UTF-8") {
            for (size_t i = 0; i < num_test_cases; i++) {
                char *str = (char *)test_cases[i]->utf8;

                char buf[512] = {0};
                char *p = buf;
                size_t len = 512;
                for (size_t j = 0; test_cases[i]->utf32[j]; j++) {
                    len -= utf8_encode(test_cases[i]->utf32[j], &p, len);
                }

                // Check that the two strings are equal. Unlike REQUIRE, CHECK
                // statements will continue the test case after encountering a
                // failure.
                CHECK_STR_EQ(buf, str);
            }
        }

    // Branch out into another subtest that can be executed independently
    SUBTEST("decode UTF-8 to UTF-32") {
            for (size_t i = 0; i < num_test_cases; i++) {
                char *str = (char *)test_cases[i]->utf8;

                for (size_t j = 0; test_cases[i]->utf32[j]; j++) {
                    CHECK_EQ(utf8_decode(&str), test_cases[i]->utf32[j]);
                }
                CHECK_EQ(utf8_decode(&str), 0);
            }
        }
}
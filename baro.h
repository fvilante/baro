#ifndef BARO_3FDC036FA2C64C72A0DB6BA1033C678B
#define BARO_3FDC036FA2C64C72A0DB6BA1033C678B

#ifdef BARO_ENABLE

#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#ifdef _WIN32
#define BARO__RED ""
#define BARO__GREEN ""
#define BARO__UNSET_COLOR ""
//#else
//#define BARO__RED "\x1B[31m"
//#define BARO__GREEN "\x1B[32m"
//#define BARO__UNSET_COLOR "\x1B[0m"
//#endif//_WIN32

struct baro__tag {
    const char *desc;
    const char *file_path;
    int line_num;
};

struct baro__tag_list {
    struct baro__tag const **tags;
    size_t size;
    size_t capacity;
};

static inline void baro__tag_list_create(
        struct baro__tag_list * const list,
        size_t const capacity) {
    list->tags = calloc(capacity, sizeof(struct baro__tag *));
    list->size = 0;
    list->capacity = capacity;
}

static inline size_t baro__tag_list_size(
        struct baro__tag_list * const list) {
    return list->size;
}

static inline void baro__tag_list_clear(
        struct baro__tag_list * const list) {
    list->size = 0;
}

static inline void baro__tag_list_push(
        struct baro__tag_list * const list,
        struct baro__tag const * const tag) {
    if (list->size == list->capacity) {
        list->capacity *= 2;

        struct baro__tag const ** const old_tags = list->tags;

        list->tags = calloc(list->capacity, sizeof(struct baro__tag *));
        for (size_t i = 0; i < list->size; i++) {
            list->tags[i] = old_tags[i];
        }

        free(old_tags);
    }

    list->tags[list->size++] = tag;
}

static inline int baro__tag_list_pop(
        struct baro__tag_list * const list,
        struct baro__tag * const tag) {
    if (list->size == 0) {
        return 0;
    }

    if (tag) {
        *tag = *list->tags[list->size - 1];
    }
    list->size--;

    return 1;
}

static inline uint64_t baro__tag_list_hash(
        struct baro__tag_list * const list) {
    uint64_t hash = 0;

    // Use the address of each item in the list as build a hash, as we only
    // expect tags to be statically allocated
    for (size_t i = 0; i < list->size; i++) {
        // Adapted from MurmurHash3's avalanche mixer
        uint64_t a = (uint64_t) list->tags[i] + i;
        a ^= a >> 33u;
        a *= 0xff51afd7ed558ccdL;
        a ^= a >> 33u;
        a *= 0xc4ceb9fe1a85ec53L;
        a ^= a >> 33u;
        hash ^= a;
    }

    return hash;
}

struct baro__hash_set {
    size_t nbits;
    uint64_t mask;

    size_t capacity;
    size_t size;

    uint64_t *hashes;
};

static inline void baro__hash_set_create(
        struct baro__hash_set * const set) {
    set->nbits = 4;
    set->capacity = 1llu << set->nbits;
    set->mask = set->capacity - 1;

    set->hashes = calloc(set->capacity, sizeof(uint64_t));
    set->size = 0;
}

static inline void baro__hash_set_clear(
        struct baro__hash_set * const set) {
    set->size = 0;
    memset(set->hashes, 0, set->capacity * sizeof(uint64_t));
}

static const uint64_t hash_set_prime_1 = 73;
static const uint64_t hash_set_prime_2 = 5009;

static inline void baro__hash_set_add_only(
        struct baro__hash_set * const set,
        uint64_t const hash) {
    uint64_t index = set->mask & (hash_set_prime_1 * hash);

    while (set->hashes[index]) {
        if (set->hashes[index] == hash) {
            return;
        }

        index = set->mask & (index + hash_set_prime_2);
    }

    set->size++;
    set->hashes[index] = hash;
}

static inline void baro__hash_set_grow(
        struct baro__hash_set * const set) {
    if ((int) set->size > (double) set->capacity * 0.85) {
        uint64_t *prev_hashes = set->hashes;
        size_t prev_capacity = set->capacity;

        set->nbits++;
        set->capacity = 1llu << set->nbits;
        set->mask = set->capacity - 1;

        set->hashes = calloc(set->capacity, sizeof(uint64_t));
        set->size = 0;
        for (size_t i = 0; i < prev_capacity; i++) {
            if (prev_hashes[i] == 0) {
                continue;
            }

            baro__hash_set_add_only(set, prev_hashes[i]);
        }
        free(prev_hashes);
    }
}

static inline void baro__hash_set_add(
        struct baro__hash_set * const set,
        uint64_t const hash) {
    baro__hash_set_add_only(set, hash);
    baro__hash_set_grow(set);
}

static inline int baro__hash_set_contains(
        struct baro__hash_set * const set,
        uint64_t const hash) {
    uint64_t index = set->mask & (hash_set_prime_1 * hash);

    while (set->hashes[index]) {
        if (set->hashes[index] == hash) {
            return 1;
        }

        index = set->mask & (index + hash_set_prime_2);
    }

    return 0;
}

struct baro__test {
    const struct baro__tag *tag;

    void (*func)(void);
};

struct baro__test_list {
    struct baro__test *tests;
    size_t size;
    size_t capacity;
};

static inline void baro__test_list_create(
        struct baro__test_list * const list,
        size_t const capacity) {
    list->tests = calloc(capacity, sizeof(struct baro__test));
    list->size = 0;
    list->capacity = capacity;
}

static inline size_t baro__test_list_size(
        struct baro__test_list * const list) {
    return list->size;
}

static inline void baro__test_list_add(
        struct baro__test_list * const list,
        struct baro__test const * const test) {
    if (list->size == list->capacity) {
        list->capacity *= 2;

        struct baro__test *old_tests = list->tests;

        list->tests = calloc(list->capacity, sizeof(struct baro__test));
        for (size_t i = 0; i < list->size; i++) {
            list->tests[i] = old_tests[i];
        }

        free(old_tests);
    }

    list->tests[list->size++] = *test;
}

static int baro__test_list_sort_cmp(
        void const *lhs,
        void const *rhs) {
    struct baro__test const * const lhs_test = lhs;
    struct baro__test const * const rhs_test = rhs;

    int const file_name_cmp = strcmp(lhs_test->tag->file_path, rhs_test->tag->file_path);
    if (file_name_cmp != 0) {
        return file_name_cmp;
    }

    return lhs_test->tag->line_num - rhs_test->tag->line_num;
}

static inline void baro__test_list_sort(
        struct baro__test_list * const list) {
    qsort(list->tests, list->size, sizeof(list->tests[0]), baro__test_list_sort_cmp);
}

// Maximum number of bytes to record from stdout per test
#define BARO__STDOUT_BUF_SIZE 4096

struct baro__context {
    struct baro__test_list tests;
    struct baro__test const *current_test;
    int current_test_failed;

    size_t num_tests_ran;
    size_t num_tests_failed;

    size_t num_asserts;
    size_t num_asserts_failed;

    // A stack that updates as we enter and exit subtests. This is mainly
    // used to build "stack traces" for assertion failures.
    struct baro__tag_list subtest_stack;
    // A set of all visited subtests in the current test, stored as 64-bit
    // hashes of the terminating subtest stacks.
    struct baro__hash_set passed_subtests;
    size_t subtest_max_size;
    int should_reenter_subtest;
    int subtest_entered;

    jmp_buf env;

    int real_stdout;
    char stdout_buffer[BARO__STDOUT_BUF_SIZE];
};

extern struct baro__context baro__c;

static inline void baro__context_create(
        struct baro__context * const context) {
    baro__test_list_create(&context->tests, 128);
    context->current_test = NULL;
    context->current_test_failed = 0;

    context->num_tests_ran = context->num_tests_failed = 0;
    context->num_asserts = context->num_asserts_failed = 0;

    baro__tag_list_create(&context->subtest_stack, 8);
    baro__hash_set_create(&context->passed_subtests);
    context->subtest_max_size = 0;
    context->should_reenter_subtest = 0;
    context->subtest_entered = 0;

    context->real_stdout = -1;
    memset(context->stdout_buffer, 0, BARO__STDOUT_BUF_SIZE);
}

#ifdef _WIN32
#include <io.h>

#define dup _dup
#define dup2 _dup2
#define strcasecmp _stricmp
#define fileno _fileno
#else
#include <unistd.h>
#endif

static inline void baro__redirect_output(
        struct baro__context * const context,
        int const enable) {
    if (enable) {
        fflush(stdout);
        context->real_stdout = dup(fileno(stdout));
#ifdef _WIN32
        FILE *dummy;
        if (freopen_s(&dummy, "NUL", "a", stdout) != 0) {
#else
        if (freopen("/dev/null", "a", stdout) == NULL) {
#endif
            fprintf(stderr, "Failed to redirect stdout\n");
            exit(1);
        }
        setvbuf(stdout, baro__c.stdout_buffer, _IOFBF, BARO__STDOUT_BUF_SIZE);
    } else if (context->real_stdout != -1) {
#ifdef _WIN32
        FILE *dummy;
        if (freopen_s(&dummy, "NUL", "a", stdout) != 0) {
#else
        if (freopen("/dev/null", "a", stdout) == NULL) {
#endif
            fprintf(stderr, "Failed to redirect stdout\n");
            exit(1);
        }
        dup2(context->real_stdout, fileno(stdout));
        setvbuf(stdout, NULL, _IONBF, 0);
    }
}

static inline void baro__register_test(
        void (* const test_func)(void),
        struct baro__tag const * const tag) {
    if (baro__test_list_size(&baro__c.tests) == 0) {
        baro__context_create(&baro__c);
    }

    struct baro__test const test = {.func = test_func, .tag = tag};
    baro__test_list_add(&baro__c.tests, &test);
}

static inline int baro__check_subtest(
        struct baro__tag const * const tag) {
    if (baro__tag_list_size(&baro__c.subtest_stack) < baro__c.subtest_max_size) {
        baro__c.should_reenter_subtest = 1;
        return 0;
    }

    baro__tag_list_push(&baro__c.subtest_stack, tag);
    if (baro__hash_set_contains(&baro__c.passed_subtests,
                                baro__tag_list_hash(&baro__c.subtest_stack))) {
        baro__tag_list_pop(&baro__c.subtest_stack, NULL);
        return 0;
    }

    baro__c.subtest_max_size = baro__c.subtest_stack.size;
    baro__c.subtest_entered = 1;
    return 1;
}

static inline void baro__exit_subtest(void) {
    if (baro__c.subtest_entered) {
        if (!baro__c.should_reenter_subtest) {
            baro__hash_set_add(&baro__c.passed_subtests, baro__tag_list_hash(&baro__c.subtest_stack));
        }

        baro__tag_list_pop(&baro__c.subtest_stack, NULL);
    }
}

#define BARO__SEPARATOR "============================================================\n"

enum baro__assert_type {
    BARO__ASSERT_CHECK,
    BARO__ASSERT_REQUIRE,
};

enum baro__assert_cond {
    BARO__ASSERT_EQ,
    BARO__ASSERT_NE,
    BARO__ASSERT_LT,
    BARO__ASSERT_LE,
    BARO__ASSERT_GT,
    BARO__ASSERT_GE,
};

enum baro__case_sensitivity {
    BARO__CASE_SENSITIVE,
    BARO__CASE_INSENSITIVE,
};

enum baro__expected_value {
    BARO__EXPECTING_FALSE,
    BARO__EXPECTING_TRUE,
};

static char const *extract_file_name(
        char const *path) {
    char const *last = path;
    for (char const *p = path; *p; p++) {
        if ((*p == '/' || *p == '\\' || *p == ':') && (p[1] != '\0')) {
            last = p + 1;
        }
    }
    return last;
}

static inline void baro__assert_failed(
        enum baro__assert_type const type) {
    struct baro__test const * const test = baro__c.current_test;
    printf("  In: %s (%s:%d)\n",
           test->tag->desc, extract_file_name(test->tag->file_path), test->tag->line_num);

    for (size_t i = 0; i < baro__c.subtest_stack.size; i++) {
        struct baro__tag const * const subtest_tag = baro__c.subtest_stack.tags[i];
        printf("%*cUnder: %s (%s:%d)\n", (int) (i + 2) * 2, ' ',
               subtest_tag->desc, extract_file_name(subtest_tag->file_path), subtest_tag->line_num);
    }

    if (baro__c.stdout_buffer[0]) {
        printf("Captured output:\n%s\n", baro__c.stdout_buffer);

        memset(baro__c.stdout_buffer, 0, BARO__STDOUT_BUF_SIZE);
    }

    printf(BARO__SEPARATOR);

    baro__redirect_output(&baro__c, 1);

    if (type == BARO__ASSERT_REQUIRE) {
        longjmp(baro__c.env, 1);
    }
}

static inline void baro__assert1(
        size_t const value,
        char const * const value_str,
        enum baro__expected_value const expected_value,
        enum baro__assert_type const type,
        char const * const desc,
        char const * const file_path,
        int const line_num) {
    baro__c.num_asserts++;

    if ((value != 0) == (expected_value == BARO__EXPECTING_TRUE)) {
        return;
    }

    baro__c.current_test_failed = 1;
    baro__c.num_asserts_failed++;

    baro__redirect_output(&baro__c, 0);

    char const * const assert_type = (type == BARO__ASSERT_REQUIRE ? "Require" : "Check");
    char const * const op = (expected_value == BARO__EXPECTING_TRUE ? " != 0" : " == 0");
    printf(BARO__RED "%s failed:%s\n" BARO__UNSET_COLOR, assert_type, desc);
    printf("    %s%s\n", value_str, op);
    printf("==> %zu%s\n", value, op);
    printf("At %s:%d\n", extract_file_name(file_path), line_num);

    baro__assert_failed(type);
}

static inline void baro__assert2(
        enum baro__assert_cond cond,
        size_t lhs,
        const char *lhs_str,
        size_t rhs,
        const char *rhs_str,
        enum baro__assert_type type,
        const char *desc,
        const char *file_path,
        int line_num) {
    baro__c.num_asserts++;

    if ((cond == BARO__ASSERT_EQ && lhs == rhs) ||
        (cond == BARO__ASSERT_NE && lhs != rhs) ||
        (cond == BARO__ASSERT_LT && lhs < rhs) ||
        (cond == BARO__ASSERT_LE && lhs <= rhs) ||
        (cond == BARO__ASSERT_GT && lhs > rhs) ||
        (cond == BARO__ASSERT_GE && lhs >= rhs)) {
        return;
    }

    baro__c.current_test_failed = 1;
    baro__c.num_asserts_failed++;

    baro__redirect_output(&baro__c, 0);

    char const * const op =
            cond == BARO__ASSERT_EQ ? "==" :
            cond == BARO__ASSERT_NE ? "!=" :
            cond == BARO__ASSERT_LT ? "<" :
            cond == BARO__ASSERT_LE ? "<=" :
            cond == BARO__ASSERT_GT ? ">" :
            cond == BARO__ASSERT_GE ? ">=" : "";

    char const * const assert_type = (type == BARO__ASSERT_REQUIRE ? "Require" : "Check");
    printf(BARO__RED "%s failed:%s\n" BARO__UNSET_COLOR, assert_type, desc);
    printf("    %s %s %s\n", lhs_str, op, rhs_str);
    printf("==> %zu %s %zu\n", lhs, op, rhs);
    printf("At %s:%d\n", extract_file_name(file_path), line_num);

    baro__assert_failed(type);
}

static inline void baro__assert_str(
        const char *lhs,
        const char *lhs_str,
        char const *rhs,
        char const *rhs_str,
        enum baro__expected_value expected_value,
        enum baro__case_sensitivity case_sensitivity,
        enum baro__assert_type type,
        char const *desc,
        char const *file_path,
        int line_num) {
    baro__c.num_asserts++;

    if ((case_sensitivity == BARO__CASE_SENSITIVE && (strcmp(lhs, rhs) == 0) == (expected_value == BARO__EXPECTING_TRUE)) ||
        (case_sensitivity == BARO__CASE_INSENSITIVE && (strcasecmp(lhs, rhs) == 0) == (expected_value == BARO__EXPECTING_TRUE))) {
        return;
    }

    baro__c.current_test_failed = 1;
    baro__c.num_asserts_failed++;

    baro__redirect_output(&baro__c, 0);

    char const * const op = (expected_value == BARO__EXPECTING_TRUE ? "==" : "!=");
    char const * const assert_type = (type == BARO__ASSERT_REQUIRE ? "Require" : "Check");
    char const * const sensitivity = (case_sensitivity == BARO__CASE_SENSITIVE ? "" : " (case insensitive)");

    char const * const lhs_wrap = (lhs ? "\"" : "");
    char const * const rhs_wrap = (rhs ? "\"" : "");
    if (!lhs) {
        lhs = "[null]";
    }
    if (!rhs) {
        rhs = "[null]";
    }

    unsigned int const str_len = strlen(lhs_str);
    unsigned int const expanded_len = strlen(lhs) + strlen(lhs_wrap) * 2;

    unsigned int str_padding = 0;
    unsigned int expanded_padding = 0;
    if (str_len > expanded_len) {
        expanded_padding = str_len - expanded_len;
    } else if (expanded_len > str_len) {
        str_padding = expanded_len - str_len;
    }

    printf(BARO__RED "%s%s failed:%s\n" BARO__UNSET_COLOR, assert_type, sensitivity, desc);
    printf("    %s %*s%s %s\n", lhs_str, str_padding, "", op, rhs_str);
    printf("==> %s%s%s %*s%s %s%s%s\n", lhs_wrap, lhs, lhs_wrap, expanded_padding, "", op, rhs_wrap, rhs, rhs_wrap);
    printf("At %s:%d\n", extract_file_name(file_path), line_num);

    baro__assert_failed(type);
}

#else
#define baro__assert1(...)
#define baro__assert2(...)
#define baro__assert_str(...) ;
#endif//BARO_ENABLE

#define BARO__CONCAT(a, b) BARO__CONCAT_INTERNAL(a, b)
#define BARO__CONCAT_INTERNAL(a, b) a##b

#define BARO__WITH_COUNTER(x) BARO__CONCAT(x, __COUNTER__)

#ifdef _MSC_VER
// MSVC doesn't support the constructor attribute. Instead, we can place our
// function in one of the CRT initializer segments, so that it still gets
// called before `main`. The `C` in `XCU` indicates it will run with the other
// C++ initializers (and after C initializers), while the `U` puts it after any
// MSVC-generated initializers, like for globals. See the CRT's `defects.inc`
// for "documentation" of this behavior.
#define BARO__INITIALIZER(f)                                                   \
    static void f(void);                                                       \
    __pragma(data_seg(".CRT$XCU"))                                             \
    __declspec(allocate(".CRT$XCU")) static void (* const f##_init)(void) = f; \
    __pragma(data_seg())                                                       \
    static void f(void)
#else
#define BARO__INITIALIZER(f) \
    __attribute__((constructor)) static void f(void)
#endif//_MSC_VER

// All test functions are registered by a "registrar function" sometime during
// runtime initialization. This is used to automatically build a list of all
// tests, across compilation units, for the test runner.
#define BARO__CREATE_TEST_REGISTRAR(func_name, desc)                            \
    static struct baro__tag const func_name##_tag = {desc, __FILE__, __LINE__}; \
    BARO__INITIALIZER(func_name##_registrar) {                                  \
        baro__register_test(func_name, &func_name##_tag);                       \
    }

#ifdef BARO_ENABLE
#define BARO__TEST_FUNC(func_name, desc)         \
    static void func_name(void);                 \
    BARO__CREATE_TEST_REGISTRAR(func_name, desc) \
    static void func_name(void)
#else
#define BARO__TEST_FUNC(func_name, ...) \
    static void __attribute__((unused)) func_name(void)
#endif//BARO_ENABLE

#define BARO_TEST(desc) BARO__TEST_FUNC(BARO__WITH_COUNTER(BARO_TEST_), desc)

#ifdef BARO_ENABLE
// Here we abuse a while loop so that our macro can call functions before and
// after any arbitrary block of code. This allows us to check if a subtest
// should be executed (i.e. if we haven't exhausted all combinations including
// it), while also updating the stack once we leave the subtest.
#define BARO__SUBTEST_WRAPPER(desc, counter)                                                                                 \
    static struct baro__tag const BARO__CONCAT(baro__subtest_tag_, counter) = {desc, __FILE__, __LINE__};                    \
    int const BARO__CONCAT(baro__enter_subtest_, counter) = baro__check_subtest(&BARO__CONCAT(baro__subtest_tag_, counter)); \
    if (BARO__CONCAT(baro__enter_subtest_, counter)) goto BARO__CONCAT(baro__subtest_, counter);                             \
    while (BARO__CONCAT(baro__enter_subtest_, counter))                                                                      \
        if (1) {                                                                                                             \
            baro__exit_subtest();                                                                                            \
            break;                                                                                                           \
        } else                                                                                                               \
            BARO__CONCAT(baro__subtest_, counter) :
#else
#define BARO__SUBTEST_WRAPPER(...)
#endif//BARO_ENABLE

#define BARO_SUBTEST(desc) BARO__SUBTEST_WRAPPER(desc, __COUNTER__)

#define BARO__CHECK1(cond) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_TRUE, BARO__ASSERT_CHECK, "", __FILE__, __LINE__)
#define BARO__CHECK2(cond, desc) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_TRUE, BARO__ASSERT_CHECK, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE1(cond) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_TRUE, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE2(cond, desc) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_TRUE, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_FALSE1(cond) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_FALSE, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_FALSE2(cond, desc) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_FALSE, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_FALSE1(cond) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_FALSE, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_FALSE2(cond, desc) baro__assert1((size_t)cond, #cond, BARO__EXPECTING_FALSE, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_EQ1(lhs, rhs) baro__assert2(BARO__ASSERT_EQ, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_EQ2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_EQ, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_EQ1(lhs, rhs) baro__assert2(BARO__ASSERT_EQ, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_EQ2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_EQ, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_NE1(lhs, rhs) baro__assert2(BARO__ASSERT_NE, (size_t)lhs, #lhs, rhs, #rhs, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_NE2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_NE, (size_t)lhs, #lhs, rhs, #rhs, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_NE1(lhs, rhs) baro__assert2(BARO__ASSERT_NE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_NE2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_NE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_LT1(lhs, rhs) baro__assert2(BARO__ASSERT_LT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_LT2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_LT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_LT1(lhs, rhs) baro__assert2(BARO__ASSERT_LT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_LT2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_LT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_LE1(lhs, rhs) baro__assert2(BARO__ASSERT_LE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_LE2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_LE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_LE1(lhs, rhs) baro__assert2(BARO__ASSERT_LE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_LE2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_LE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_GT1(lhs, rhs) baro__assert2(BARO__ASSERT_GT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_GT2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_GT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_GT1(lhs, rhs) baro__assert2(BARO__ASSERT_GT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_GT2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_GT, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_GE1(lhs, rhs) baro__assert2(BARO__ASSERT_GE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_GE2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_GE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_GE1(lhs, rhs) baro__assert2(BARO__ASSERT_GE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_GE2(lhs, rhs, desc) baro__assert2(BARO__ASSERT_GE, (size_t)lhs, #lhs, (size_t)rhs, #rhs, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_STR_EQ2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_SENSITIVE, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_STR_EQ3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_SENSITIVE, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_STR_EQ2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_SENSITIVE, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_STR_EQ3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_SENSITIVE, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_STR_NE2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_SENSITIVE, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_STR_NE3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_SENSITIVE, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_STR_NE2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_SENSITIVE, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_STR_NE3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_SENSITIVE, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_STR_ICASE_EQ2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_INSENSITIVE, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_STR_ICASE_EQ3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_INSENSITIVE, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_STR_ICASE_EQ2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_INSENSITIVE, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_STR_ICASE_EQ3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_TRUE, BARO__CASE_INSENSITIVE, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__CHECK_STR_ICASE_NE2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_INSENSITIVE, 0, "", __FILE__, __LINE__)
#define BARO__CHECK_STR_ICASE_NE3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_INSENSITIVE, 0, " " desc, __FILE__, __LINE__)

#define BARO__REQUIRE_STR_ICASE_NE2(lhs, rhs) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_INSENSITIVE, BARO__ASSERT_REQUIRE, "", __FILE__, __LINE__)
#define BARO__REQUIRE_STR_ICASE_NE3(lhs, rhs, desc) baro__assert_str(lhs, #lhs, rhs, #rhs, BARO__EXPECTING_FALSE, BARO__CASE_INSENSITIVE, BARO__ASSERT_REQUIRE, " " desc, __FILE__, __LINE__)

#define BARO__GET2(_1, _2, NAME, ...) NAME
#define BARO__GET3(_1, _2, _3, NAME, ...) NAME

#ifdef _MSC_VER
#define BARO__X(x) x
#define BARO_CHECK(...) BARO__X(BARO__GET2(__VA_ARGS__, BARO__CHECK2, BARO__CHECK1)) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE(...) BARO__X(BARO__GET2(__VA_ARGS__, BARO__REQUIRE2, BARO__REQUIRE1)) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_FALSE(...) BARO__X(BARO__GET2(__VA_ARGS__, BARO__CHECK_FALSE2, BARO__CHECK_FALSE1)) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_FALSE(...) BARO__X(BARO__GET2(__VA_ARGS__, BARO__REQUIRE_FALSE2, BARO__REQUIRE_FALSE1)) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_EQ(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_EQ2, BARO__CHECK_EQ1, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_EQ(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_EQ2, BARO__REQUIRE_EQ1, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_NE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_NE2, BARO__CHECK_NE1, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_NE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_NE2, BARO__REQUIRE_NE1, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_LT(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_LT2, BARO__CHECK_LT1, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_LT(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_LT2, BARO__REQUIRE_LT1, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_LE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_LE2, BARO__CHECK_LE1, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_LE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_LE2, BARO__REQUIRE_LE1, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_GT(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_GT2, BARO__CHECK_GT1, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_GT(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_GT2, BARO__REQUIRE_GT1, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_GE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_GE2, BARO__CHECK_GE1, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_GE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_GE2, BARO__REQUIRE_GE1, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_STR_EQ(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_EQ3, BARO__CHECK_STR_EQ2, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_STR_EQ(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_EQ3, BARO__REQUIRE_STR_EQ2, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_STR_NE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_NE3, BARO__CHECK_STR_NE2, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_STR_NE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_NE3, BARO__REQUIRE_STR_NE2, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_STR_ICASE_EQ(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_ICASE_EQ3, BARO__CHECK_STR_ICASE_EQ2, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_STR_ICASE_EQ(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_ICASE_EQ3, BARO__REQUIRE_STR_ICASE_EQ2, )) \
BARO__X((__VA_ARGS__))
#define BARO_CHECK_STR_ICASE_NE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_ICASE_NE3, BARO__CHECK_STR_ICASE_NE2, )) \
BARO__X((__VA_ARGS__))
#define BARO_REQUIRE_STR_ICASE_NE(...) BARO__X(BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_ICASE_NE3, BARO__REQUIRE_STR_ICASE_NE2, )) \
BARO__X((__VA_ARGS__))
#else
#define BARO_CHECK(...) BARO__GET2(__VA_ARGS__, BARO__CHECK2, BARO__CHECK1) \
(__VA_ARGS__)
#define BARO_REQUIRE(...) BARO__GET2(__VA_ARGS__, BARO__REQUIRE2, BARO__REQUIRE1) \
(__VA_ARGS__)
#define BARO_CHECK_FALSE(...) BARO__GET2(__VA_ARGS__, BARO__CHECK_FALSE2, BARO__CHECK_FALSE1) \
(__VA_ARGS__)
#define BARO_REQUIRE_FALSE(...) BARO__GET2(__VA_ARGS__, BARO__REQUIRE_FALSE2, BARO__REQUIRE_FALSE1) \
(__VA_ARGS__)
#define BARO_CHECK_EQ(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_EQ2, BARO__CHECK_EQ1, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_EQ(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_EQ2, BARO__REQUIRE_EQ1, ) \
(__VA_ARGS__)
#define BARO_CHECK_NE(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_NE2, BARO__CHECK_NE1, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_NE(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_NE2, BARO__REQUIRE_NE1, ) \
(__VA_ARGS__)
#define BARO_CHECK_LT(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_LT2, BARO__CHECK_LT1, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_LT(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_LT2, BARO__REQUIRE_LT1, ) \
(__VA_ARGS__)
#define BARO_CHECK_LE(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_LE2, BARO__CHECK_LE1, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_LE(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_LE2, BARO__REQUIRE_LE1, ) \
(__VA_ARGS__)
#define BARO_CHECK_GT(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_GT2, BARO__CHECK_GT1, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_GT(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_GT2, BARO__REQUIRE_GT1, ) \
(__VA_ARGS__)
#define BARO_CHECK_GE(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_GE2, BARO__CHECK_GE1, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_GE(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_GE2, BARO__REQUIRE_GE1, ) \
(__VA_ARGS__)
#define BARO_CHECK_STR_EQ(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_EQ3, BARO__CHECK_STR_EQ2, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_STR_EQ(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_EQ3, BARO__REQUIRE_STR_EQ2, ) \
(__VA_ARGS__)
#define BARO_CHECK_STR_NE(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_NE3, BARO__CHECK_STR_NE2, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_STR_NE(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_NE3, BARO__REQUIRE_STR_NE2, ) \
(__VA_ARGS__)
#define BARO_CHECK_STR_ICASE_EQ(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_ICASE_EQ3, BARO__CHECK_STR_ICASE_EQ2, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_STR_ICASE_EQ(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_ICASE_EQ3, BARO__REQUIRE_STR_ICASE_EQ2, ) \
(__VA_ARGS__)
#define BARO_CHECK_STR_ICASE_NE(...) BARO__GET3(__VA_ARGS__, BARO__CHECK_STR_ICASE_NE3, BARO__CHECK_STR_ICASE_NE2, ) \
(__VA_ARGS__)
#define BARO_REQUIRE_STR_ICASE_NE(...) BARO__GET3(__VA_ARGS__, BARO__REQUIRE_STR_ICASE_NE3, BARO__REQUIRE_STR_ICASE_NE2, ) \
(__VA_ARGS__)
#endif//_MSC_VER

#ifndef BARO_NO_SHORT
#define TEST BARO_TEST
#define SUBTEST BARO_SUBTEST
#define CHECK BARO_CHECK
#define REQUIRE BARO_REQUIRE
#define CHECK_FALSE BARO_CHECK_FALSE
#define REQUIRE_FALSE BARO_REQUIRE_FALSE
#define CHECK_EQ BARO_CHECK_EQ
#define REQUIRE_EQ BARO_REQUIRE_EQ
#define CHECK_NE BARO_CHECK_NE
#define REQUIRE_NE BARO_REQUIRE_NE
#define CHECK_LT BARO_CHECK_LT
#define REQUIRE_LT BARO_REQUIRE_LT
#define CHECK_LE BARO_CHECK_LE
#define REQUIRE_LE BARO_REQUIRE_LE
#define CHECK_GT BARO_CHECK_GT
#define REQUIRE_GT BARO_REQUIRE_GT
#define CHECK_GE BARO_CHECK_GE
#define REQUIRE_GE BARO_REQUIRE_GE
#define CHECK_STR_EQ BARO_CHECK_STR_EQ
#define REQUIRE_STR_EQ BARO_CHECK_STR_EQ
#define CHECK_STR_NE BARO_CHECK_STR_NE
#define REQUIRE_STR_NE BARO_REQUIRE_STR_NE
#define CHECK_STR_ICASE_EQ BARO_CHECK_STR_ICASE_EQ
#define REQUIRE_STR_ICASE_EQ BARO_REQUIRE_STR_ICASE_EQ
#define CHECK_STR_ICASE_NE BARO_CHECK_STR_ICASE_NE
#define REQUIRE_STR_ICASE_NE BARO_REQUIRE_STR_ICASE_NE
#endif//BARO_NO_SHORT
#endif//BARO_3FDC036FA2C64C72A0DB6BA1033C678B

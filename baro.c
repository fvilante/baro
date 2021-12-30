#include "baro.h"

struct baro__context baro__c = {0};

char *optarg;

// A small getopt-like function for parsing short CLI arguments
int getopt(
        int const num_args,
        char * const * args,
        char const * opts) {
    static char *arg = "";
    static int cur_arg = 1;

    if (!*arg) {
        if (cur_arg >= num_args || *(arg = args[cur_arg]) != '-') {
            arg = "";
            return -1;
        }
        if (arg[1] && *++arg == '-') {
            ++cur_arg;
            arg = "";
            fprintf(stderr, "Long options not supported\n");
            return 0;
        }
    }

    char opt;
    char const *cur_opt;

    if ((opt = *arg++) == ':' || !(cur_opt = strchr(opts, opt))) {
        if (opt == '-') {
            return -1;
        }
        if (!*arg) {
            ++cur_arg;
        }
        fprintf(stderr, "Illegal option: %c\n", opt);
        return 0;
    }

    if (*++cur_opt != ':') {
        optarg = NULL;
        if (!*arg) {
            ++cur_arg;
        }
    } else {
        if (*arg) {
            optarg = arg;
        } else if (num_args <= ++cur_arg) {
            arg = "";
            fprintf(stderr, "Option requires an argument: %c\n", opt);
            return 0;
        } else {
            optarg = args[cur_arg];
        }
        arg = "";
        ++cur_arg;
    }

    return opt;
}

int main(
        int argc,
        char *argv[]) {
    int show_passed_tests = 0;
    int suppress_output = 1;
    int stop_after_failure = 0;
    size_t num_partitions = 1;
    size_t cur_partition = 1;

    // Parse command line options
    int c;
    while ((c = getopt(argc, argv, "haosp:n:")) != -1) {
        switch (c) {
        case 'p':
            num_partitions = strtol(optarg, NULL, 10);
            break;

        case 'n':
            cur_partition = strtol(optarg, NULL, 10);
            break;

        case 'h':
            printf("baro unit tests\n");
            return 0;

        case 'a':
            show_passed_tests = 1;
            break;

        case 'o':
            suppress_output = 0;
            break;

        case 's':
            stop_after_failure = 1;
            break;

        default:
            fprintf(stderr, "Unknown arguments: run with -h for help\n");
            return -1;
        }
    }

    size_t const num_tests = baro__c.tests.size;
    if (num_tests > 0 && (num_partitions < 1 || num_partitions > num_tests)) {
        fprintf(stderr, "Invalid number of partitions %zu, value should be"
                        "between 1 and %zu\n", num_partitions, num_tests);
        return -1;
    }

    if (cur_partition < 1 || cur_partition > num_partitions) {
        fprintf(stderr, "Invalid current partition %zu, value should between 1"
                        " and %zu inclusive\n", cur_partition, num_partitions);
        return -1;
    }

    // Sort the list of tests so that we get a deterministic order of execution
    // across different compilers and runtimes
    baro__test_list_sort(&baro__c.tests);

    // Partition the tests if we are in a multiprocess workflow
    size_t const partition_size = (num_tests + (num_partitions - 1)) / num_partitions;
    size_t const first_test = partition_size * (cur_partition - 1);
    size_t last_test = first_test + partition_size;
    if (last_test >= num_tests) {
        last_test = num_tests;
    }

    size_t const num_tests_to_run = last_test - first_test;
    printf("Running %zu out of %zu test%s\n", num_tests_to_run, num_tests,
           num_tests > 1 ? "s" : "");
    if (num_partitions > 1) {
        printf("(Partition %zu: tests %zu to %zu)\n", cur_partition, first_test, last_test - 1);
    }

    printf(BARO__SEPARATOR);

    baro__redirect_output(&baro__c, suppress_output);

    // Begin running tests serially
    for (size_t i = first_test; i < last_test; i++) {
        struct baro__test const * const test = &baro__c.tests.tests[i];
        baro__c.current_test = test;
        baro__c.current_test_failed = 0;
        baro__hash_set_clear(&baro__c.passed_subtests);

        int run_test = 1;

        // Recover from required assertion failures
        if (setjmp(baro__c.env)) {
            run_test = 0;
        }

        while (run_test) {

            // Reset the current subtest stack
            baro__c.should_reenter_subtest = 0;
            baro__c.subtest_max_size = 0;
            baro__tag_list_clear(&baro__c.subtest_stack);

            test->func();

            // Keep looping until all subtest permutations have been visited
            if (!baro__c.should_reenter_subtest) {
                run_test = 0;
            }
        }

        baro__c.num_tests_ran++;
        if (baro__c.current_test_failed) {
            baro__c.num_tests_failed++;
            if (stop_after_failure) {
                break;
            }
        } else if (show_passed_tests) {
            baro__redirect_output(&baro__c, 0);

            printf(BARO__GREEN "Passed: %s (%s:%d)\n" BARO__UNSET_COLOR BARO__SEPARATOR,
                   test->tag->desc, extract_file_name(test->tag->file_path), test->tag->line_num);
            baro__redirect_output(&baro__c, suppress_output);
        }

        // Wipe the saved output between tests
        memset(baro__c.stdout_buffer, 0, BARO__STDOUT_BUF_SIZE);
    }

    baro__redirect_output(&baro__c, 0);

    printf("tests:   %5zu total | " BARO__GREEN "%5zu passed" BARO__UNSET_COLOR
           " | " BARO__RED "%5zu failed" BARO__UNSET_COLOR "\n",
           baro__c.num_tests_ran, baro__c.num_tests_ran - baro__c.num_tests_failed,
           baro__c.num_tests_failed);

    printf("asserts: %5zu total | " BARO__GREEN "%5zu passed" BARO__UNSET_COLOR
           " | " BARO__RED "%5zu failed" BARO__UNSET_COLOR "\n",
           baro__c.num_asserts, baro__c.num_asserts - baro__c.num_asserts_failed,
           baro__c.num_asserts_failed);

    return (int) baro__c.num_tests_failed;
}

#include <baro.h>

TEST("i like subtests") {
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
}
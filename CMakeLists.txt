cmake_minimum_required(VERSION 3.12)
project(baro C)

set(CMAKE_C_STANDARD 99)

enable_testing()

add_executable(baro-test baro.c baro_test.c)
target_compile_definitions(baro-test PRIVATE BARO_ENABLE BARO_SELF_TEST)

add_test(self-tests baro-test)

foreach(test basic)
    add_executable("example-${test}" "examples/${test}.c" baro.c)
    target_compile_definitions("example-${test}" PRIVATE BARO_ENABLE)
    target_include_directories("example-${test}" PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_custom_command(
        TARGET "example-${test}"
        POST_BUILD
        COMMAND "example-${test}" -a > "${test}.txt" || exit 0
    )

    add_test(
        NAME "check-example-${test}"
        COMMAND ${CMAKE_COMMAND} -E compare_files "${test}.txt" "${CMAKE_CURRENT_SOURCE_DIR}/examples/${test}.txt"
    )
endforeach()

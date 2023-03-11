cmake_minimum_required(VERSION 3.12)
project(baro C)

set(CMAKE_C_STANDARD 99)

enable_testing()

add_executable(baro_test baro.c baro_test.c)
target_compile_definitions(baro_test PRIVATE BARO_ENABLE BARO_SELF_TEST)
if(MSVC)
    # TODO remove static linking once ASan is in the PATH
    target_compile_options(baro_test PRIVATE /MTd /fsanitize=address)
    target_link_options(baro_test PRIVATE /fsanitize=address)
else()
    target_compile_options(baro_test PRIVATE -fsanitize=address,undefined)
    target_link_options(baro_test PRIVATE -fsanitize=address,undefined)
endif()

add_test(self_tests baro_test)

macro(AddExampleTest test flags)
    add_executable(
        "example_${test}"
        "examples/${test}.c" baro.c)
    target_compile_definitions("example_${test}" PRIVATE BARO_ENABLE)
    target_include_directories("example_${test}" PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    if(MSVC)
        # TODO remove static linking once ASan is in the PATH
        target_compile_options("example_${test}" PRIVATE /MTd /fsanitize=address)
        target_link_options("example_${test}" PRIVATE /fsanitize=address)
    else()
        target_compile_options("example_${test}" PRIVATE -fsanitize=address,undefined)
        target_link_options("example_${test}" PRIVATE -fsanitize=address,undefined)
    endif()

    add_custom_command(
        TARGET "example_${test}"
        POST_BUILD
        COMMAND "example_${test}" "${flags}" > "${test}.txt" || (exit 0))

    find_program(DIFF diff)
    if(DIFF)
        add_test(
                NAME "check_example_${test}"
                COMMAND diff -c --strip-trailing-cr "${test}.txt" "${CMAKE_CURRENT_SOURCE_DIR}/examples/${test}.txt")
    else()
        add_test(
                NAME "check_example_${test}"
                COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol "${test}.txt" "${CMAKE_CURRENT_SOURCE_DIR}/examples/${test}.txt")
    endif()
endmacro()

AddExampleTest(basic -a)
AddExampleTest(subtests -ao)
AddExampleTest(unicode_encoding -a)

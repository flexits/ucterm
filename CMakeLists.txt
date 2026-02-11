cmake_minimum_required(VERSION 3.20)
project(ucterm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------
# 1. Production code
# ------------------------------------------------------------------
# Collect all .c files in src (excluding main.c)
file(GLOB PROJECT_SOURCES CONFIGURE_DEPENDS *.c)
list(FILTER PROJECT_SOURCES EXCLUDE REGEX "main\\.c$")

# Build a library from production code
add_library(core STATIC ${PROJECT_SOURCES})
target_include_directories(core PUBLIC .)

# ------------------------------------------------------------------
# 2. Main executable
# ------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.c")
    add_executable(main_exec main.c)
    target_link_libraries(main_exec PRIVATE core)

    set_target_properties(main_exec PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# ------------------------------------------------------------------
# 3. Unit test executable
# ------------------------------------------------------------------
# Collect all test source files
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/test_*.c)
set(UNITY_SOURCE tests/unity/unity.c)

add_executable(unit_tests ${TEST_SOURCES} ${UNITY_SOURCE})
target_link_libraries(unit_tests PRIVATE core)
target_include_directories(unit_tests PRIVATE tests/unity)

# ------------------------------------------------------------------
# 4. Enable CTest
# ------------------------------------------------------------------
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)

# ------------------------------------------------------------------
# 5. Optional: set build output directories
# ------------------------------------------------------------------
if(TARGET main_exec)
    set_target_properties(main_exec PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

set_target_properties(unit_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

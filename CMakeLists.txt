cmake_minimum_required(VERSION 3.7)

project(Variant CXX)

option(ENABLE_VARIANT_TESTS
    "Enable tests for ${PROJECT_NAME}"
    OFF
)

option(SKIP_SUPERBUILD
    "Superbuild!"
    OFF
)

list(APPEND
    CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/cmake
)

list(APPEND
    CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/externals/cmake
)

find_package(Sanitizers)

if(NOT SKIP_SUPERBUILD)
    include(SuperBuild)
    set(SKIP_SUPERBUILD ON)
    return()
endif()

add_subdirectory(include)

if(ENABLE_VARIANT_TESTS)
    enable_testing()
    add_subdirectory(tests)
    add_subdirectory(examples)
endif()


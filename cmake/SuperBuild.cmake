include(ExternalProject)

find_package(Catch2 QUIET)
if(NOT Catch2_FOUND)
    ExternalProject_Add(
        Catch2External
        GIT_REPOSITORY https://github.com/CatchOrg/Catch2.git
        GIT_SHALLOW ON
        BUILD_ALWAYS ON
        INSTALL_DIR ${CMAKE_INSTALL_PREFIX}
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
            -DCMAKE_PREFIX_PATH=<INSTALL_DIR>
    )
else()
    add_custom_target(Catch2External)
endif()

ExternalProject_Add(
    VariantSuper
    DEPENDS Catch2External
    SOURCE_DIR ${PROJECT_SOURCE_DIR}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
    INSTALL_COMMAND ""
    CMAKE_ARGS
        -DSKIP_SUPERBUILD=ON
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
)

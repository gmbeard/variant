add_library(variant INTERFACE)
add_library(Variant::variant ALIAS variant)

target_include_directories(
    variant
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<INSTALL_INTERFACE:include>
)

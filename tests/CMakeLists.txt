find_package(Catch2 REQUIRED)

add_executable(variant_tests
    variant_tests.cpp
)

add_sanitizers(variant_tests)

target_link_libraries(variant_tests
    PRIVATE
        Variant::variant
)

target_compile_features(variant_tests
    PRIVATE
        cxx_decltype_auto
)

target_compile_options(variant_tests
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/FAsc /W4 /WX /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Werror>
)

add_test(
    NAME VariantTests
    COMMAND variant_tests
)

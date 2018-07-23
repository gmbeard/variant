add_executable(
    JsonExample
    json.cpp
)

target_link_libraries(
    JsonExample
    PRIVATE
        Variant::variant
)

add_sanitizers(JsonExample)

target_link_libraries(JsonExample
    PRIVATE
        Variant::variant
)

target_compile_features(JsonExample
    PRIVATE
        cxx_decltype_auto
)

target_compile_options(JsonExample
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/FAsc /W4 /WX /permissive->
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Werror>
)

option(ENABLE_SANITIZERS
    "Enable sanitizers for ${PROJECT_NAME}"
    OFF
)

add_library(Sanitizers INTERFACE)

if(ENABLE_SANITIZERS)
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG 
        "${CMAKE_STATIC_LINKER_FLAGS} -fsanitize=undefined"
    )

    target_compile_options(
        Sanitizers
        INTERFACE
            -fsanitize=undefined
    )
endif()

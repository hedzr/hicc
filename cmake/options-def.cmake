option(ENABLE_HICC_CLI_APP "Enable hicc cli app" ON)

if (EXISTS ${CMAKE_SOURCE_DIR}/.options.cmake)
    include(.options)
else ()
    message("   options decl file (.options.cmake) ignored")
    set(BASENAME prj)
endif ()
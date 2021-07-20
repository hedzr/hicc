

#cmake_minimum_required(VERSION 3.17)
project(hicc-ref
        VERSION ${VERSION}
        DESCRIPTION "hicc-cxx reference project here"
        LANGUAGES CXX)

debug_print_hbar()
message("!! entering subdir: ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
debug_print_value(CMAKE_SOURCE_DIR)
debug_print_value(CMAKE_CURRENT_BINARY_DIR)
#debug_dump_cmake_environments()
#debug_print_hbar()
#debug_dump_cmake_variables()
#debug_print_hbar_long()


add_executable(${PROJECT_NAME} main.cc)
target_include_directories(${PROJECT_NAME} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_GENERATED_DIR}>
        ${CMAKE_SOURCE_DIR}/include
        )
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        # Catch2::Catch2
        libs::hicc
        # cmdr11::cmdr11
        # fmt::fmt-header-only
        )


#
# Testing
#


if (${ENABLE_TESTS})
    enable_testing()
    #include(CTest) # note: this adds a BUILD_TESTING which defaults to ON

    add_subdirectory(tests)
endif ()


#
#
#


# ##########################################################


####

if (ENABLE_CMDR_CLI_TEST)
    project(cmdr-cli
            VERSION ${VERSION}
            DESCRIPTION "cmdr test cli project here"
            HOMEPAGE_URL "https://github.com/hedzr/cmdr-cxx"
            LANGUAGES CXX)

    add_executable(${PROJECT_NAME} cmdr_main.cc)
    target_include_directories(${PROJECT_NAME} PRIVATE
            $<BUILD_INTERFACE:${CMAKE_GENERATED_DIR}>
            )

    include(deps-cmdr11)
    add_cmdr_cxx_to(${PROJECT_NAME})
    #add_dependencies(${PROJECT_NAME} ${CMDR_CXX_TGT_NAME})

    message(STATUS "CMAKE_CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR}")

endif ()


# ##########################################################

#include(install-pack-helpers)
#add_install_chapter(${PROJECT_NAME})
#
#set(CPACK_PACKAGE_CONTACT "Hedzr Yeh <hedzrz@gmail.com>")
#set(CPACK_PACKAGE_VENDOR "Hedzr Studio")
#add_cpack_chapter(${PROJECT_NAME})
#
#message(STATUS "CPACK_SOURCE_PACKAGE_FILE_NAME = ${CPACK_SOURCE_PACKAGE_FILE_NAME}")
#message(STATUS "CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
#message(STATUS "CPACK_TARGET_FILE_NAME = ${CPACK_TARGET_FILE_NAME}, ${PROJECT_NAME}, ${PROJECT_VERSION}, ${CMAKE_HOST_SYSTEM_NAME}")

#
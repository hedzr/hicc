project(hicc
        VERSION ${VERSION}
        DESCRIPTION "libhicc - C++17 library to provide a couple of utilities and helpers"
        LANGUAGES CXX)


## OPTIONS ######################################

if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(_hicc_default_assertions ON)
else ()
    set(_hicc_default_assertions OFF)
endif ()

option(HICC_ENABLE_ASSERTIONS "whether or not to enable internal assertions for the hicc cxx library" ${_hicc_default_assertions})
if (${HICC_ENABLE_ASSERTIONS})
    set(_hicc_enable_assertions 1)
else ()
    set(_hicc_enable_assertions 0)
endif ()

option(HICC_ENABLE_PRECONDITION_CHECKS "whether or not to enable precondition checks" ON)
if (${HICC_ENABLE_PRECONDITION_CHECKS})
    set(_hicc_enable_precondition_checks 1)
else ()
    set(_hicc_enable_precondition_checks 0)
endif ()

option(HICC_ENABLE_WRAPPER "whether or not the wrappers in hicc-cxx.hpp are used" ON)
if (${HICC_ENABLE_WRAPPER})
    set(_hicc_enable_wrapper 1)
else ()
    set(_hicc_enable_wrapper 0)
endif ()

option(HICC_ENABLE_VERBOSE_LOG "whether or not print the matching decisions to logging output in running" OFF)
#if (${HICC_ENABLE_VERBOSE_LOG})
#    set(_hicc_enable_verbose_log 1)
#else ()
#    set(_hicc_enable_verbose_log 0)
#endif ()


## SETTINGS ###################################################

set(PROJECT_ARCHIVE_NAME ${PROJECT_NAME}-${PROJECT_VERSION})
#if (POLICY CMP0069)
#    cmake_policy(SET CMP0069 NEW)
#endif ()
if (POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif ()

# To enable Nightly Target and More, uncomment and include CTest script from CMake bundle.
# include(CTest)


#find_package(ZLIB REQUIRED)
#find_package(fmt)
#include(deps_inc_fmt)
if (${HICC_ENABLE_VERBOSE_LOG})
    # include(deps_inc_spdlog)
    # Or for a spdlog standalone install:
    # find_package(spdlog REQUIRED)
endif ()

#debug_print_hbar()
#debug_dump_cmake_environments()
#debug_print_hbar()
#debug_dump_cmake_variables()
#debug_print_hbar_long()

set(detail_header_files
        )
set(header_files
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hicc-cxx.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hicc.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-btree.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-chrono.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-dbg.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-defs.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-if.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-ios.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-log.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-mmap.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-os-io-redirect.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-path.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-pipeable.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-pool.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-priority-queue.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-process.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-ringbuf.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-string.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-terminal.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-ticker.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-util.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-x-class.hh
        ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/hz-x-test.hh
        # ${CMAKE_GENERATED_DIR}/hicc-version.hh
        )

## TARGETS ####################################################

add_library(${PROJECT_NAME} INTERFACE)
add_library(libs::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
#cxx_set_ipo(${PROJECT_NAME})
#set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "libs/${PROJECT_NAME}")


target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_11)
#target_sources(${PROJECT_NAME} PRIVATE src/${PROJECT_NAME}.cc src/private.cc)
target_sources(${PROJECT_NAME} INTERFACE "$<BUILD_INTERFACE:${detail_header_files};${header_files}>")
#set_target_properties(${PROJECT_NAME} PROPERTIES
#        COMPILE_FEATURES cxx_std_14
#        SOURCES src/${PROJECT_NAME}.cc
#        )
target_include_directories(${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        #$<INSTALL_INTERFACE:include>
        #PRIVATE
        #${ZLIB_INCLUDE_DIRS}
        #${spdlog_SOURCE_DIR}/include
        )
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)
target_compile_definitions(${PROJECT_NAME} INTERFACE
        HICC_ENABLE_ASSERTIONS=${_cmdr_enable_assertions}
        HICC_ENABLE_PRECONDITION_CHECKS=${_cmdr_enable_precondition_checks}
        HICC_ENABLE_WRAPPER=${_cmdr_enable_wrapper})
if (${HICC_ENABLE_VERBOSE_LOG})
    target_compile_definitions(${PROJECT_NAME} INTERFACE
            HICC_ENABLE_VERBOSE_LOG=1)
endif ()
#target_link_libraries(${PROJECT_NAME} INTERFACE debug_assert)
#target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
#target_link_libraries(${PROJECT_NAME} PRIVATE libs::sm-lib)
#target_link_libraries(${PROJECT_NAME} INTERFACE fmt::fmt-header-only)
if (HICC_ENABLE_VERBOSE_LOG)
    #    target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
    #target_link_libraries(${PROJECT_NAME} INTERFACE spdlog::spdlog)
    #add_dependencies(${PROJECT_NAME} spdlog)
endif ()

if (MSVC)
    target_compile_options(${PROJECT_NAME} INTERFACE /wd4800) # truncation to bool warning
endif ()


## hicc-cxx-cli Target ######################################

if (ENABLE_HICC_CLI_APP)
    set(CLI_NAME hicc-cli)
    add_executable(${CLI_NAME} src/main.cc)
    target_compile_definitions(${CLI_NAME} PRIVATE
            HICC_ENABLE_AUTO_COMPLETION_LOGFILE=1
            HICC_ENABLE_ASSERTIONS=${_cmdr_enable_assertions}
            HICC_ENABLE_PRECONDITION_CHECKS=${_cmdr_enable_precondition_checks}
            HICC_ENABLE_WRAPPER=${_cmdr_enable_wrapper})
    target_include_directories(${CLI_NAME} PRIVATE $<BUILD_INTERFACE:${CMAKE_GENERATED_DIR}>)
    target_link_libraries(${CLI_NAME} PRIVATE libs::hicc)
    set_target_properties(${CLI_NAME}
            PROPERTIES
            # DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
            RELEASE_POSTFIX "${CMAKE_RELEASE_POSTFIX}"
            MINSIZEREL_POSTFIX "${CMAKE_MINSIZEREL_POSTFIX}"
            RELWITHDEBINFO_POSTFIX "${CMAKE_RELWITHDEBINFO_POSTFIX}")
endif ()


## TESTING ####################################################

if (BUILD_TESTING)
    message(STATUS trying add test with name ${PROJECT_NAME}_tests)
    add_unit_test(${PROJECT_NAME} tests ${PROJECT_NAME}_tests)
endif ()


## INSTALLING #################################################

include(install-pack-helpers)
add_install_chapter(${PROJECT_NAME})

set(CPACK_PACKAGE_CONTACT "Hedzr Yeh <hedzrz@gmail.com>")
set(CPACK_PACKAGE_VENDOR "Hedzr Studio")
add_cpack_chapter(${PROJECT_NAME})

message(STATUS "CPACK_SOURCE_PACKAGE_FILE_NAME = ${CPACK_SOURCE_PACKAGE_FILE_NAME}")
message(STATUS "CPACK_PACKAGE_FILE_NAME = ${CPACK_PACKAGE_FILE_NAME}")
message(STATUS "CPACK_TARGET_FILE_NAME = ${CPACK_TARGET_FILE_NAME}, ${PROJECT_NAME}, ${PROJECT_VERSION}, ${CMAKE_HOST_SYSTEM_NAME}")


debug_print_value(CMAKE_PROJECT_VERSION)
debug_print_value(PROJECT_VERSION)
debug_print_value(VERSION)


if (ENABLE_HICC_CLI_APP)
    install(TARGETS ${CLI_NAME}
        EXPORT ${PROJECT_NAME}Targets
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
endif()

#install(EXPORT ${PROJECT_NAME}Targets
#        FILE ${PROJECT_NAME}Targets.cmake
#        NAMESPACE ${PROJECT_NAME}::
#        DESTINATION ${CONFIG_PACKAGE_INSTALL_DIR})
#
#install(DIRECTORY include/${PROJECT_NAME}
#        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#        )

install(FILES
        ${CMAKE_BINARY_DIR}/generated/hicc-version.hh
        DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/hicc-cxx.hh
        DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR})

message("")

#install(FILES
#        ${CMAKE_CURRENT_LIST_DIR}/cmake/${PROJECT_NAME}Config.cmake
#        DESTINATION
#        ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

#install(TARGETS ${PROJECT_NAME}
#        # COMPONENT applications
#        # DESTINATION "bin"
#        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#        )


## PACKAGING ##################################################

#add_cpack_chapter(${PROJECT_NAME})


## DEBUG ######################################################


# debug_dump_target_inc_dirs(${PROJECT_NAME})

debug_print_value(ARCHIVE_NAME)
debug_print_value(BUILD_TESTING)
debug_print_value(PROJECT_ARCHIVE_NAME)
debug_print_value(CMAKE_CURRENT_SOURCE_DIR)
debug_print_value(PROJECT_SOURCE_DIR)
debug_print_value(CPACK_PACKAGE_DIRECTORY)
debug_print_value(CPACK_PACKAGE_NAME)
debug_print_value(CPACK_PACKAGE_VERSION)
debug_print_value(CPACK_PACKAGE_VERSION_MAJOR)
debug_print_value(CPACK_PACKAGE_VERSION_MINOR)
debug_print_value(CPACK_PACKAGE_VERSION_PATCH)

##debug_dump_target_inc_dirs_interface_only(${PROJECT_NAME})
#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#foreach (dir ${dirs})
#    message(STATUS "    = '${dir}'")
#endforeach ()

#get_directory_property(inc_dirs INCLUDE_DIRECTORIES)
#list(TRANSFORM inc_dirs PREPEND "-I ")
#list(JOIN inc_dirs " " INCLUDE_STRING)
#debug_print_value(inc_dirs)

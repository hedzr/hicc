## TPL ## HEAD ##########
cmake_minimum_required(VERSION 3.9..3.19)
set(CMAKE_SCRIPTS "cmake")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/addons;${CMAKE_SOURCE_DIR}/${CMAKE_SCRIPTS}/modules;${CMAKE_SOURCE_DIR}/${CMAKE_SCRIPTS};${CMAKE_SOURCE_DIR};${CMAKE_MODULE_PATH}")
include(prerequisites)
include(version-def)
include(add-policies)
include(detect-systems)
include(target-dirs)
include(utils)

#
# OPTIONS
#

option(ENABLE_TESTS "Enable tests" ON)
option(ENABLE_AUTOMATE_TESTS "Enable automated tests at local" ON)
if (NOT ${CI_RUNNING})
    set(ENABLE_AUTOMATE_TESTS OFF)
endif ()
option(ENABLE_CMDR_CLI_TEST "Enable cmdr-cli app as a test" OFF)


#
# MAIN
#


#cmake_minimum_required(VERSION 3.17)
#project(study_cxx)
project(hicc-cxx
        VERSION ${VERSION}
        DESCRIPTION "A C++17 Common Utilities and Helpers library"
        LANGUAGES CXX)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
## set(CMAKE_CXX_EXTENSIONS OFF)
#
#enable_language(C)

set(VERSION_H_NAME "hicc-version.hh")
include(cxx-standard-def)
include(cxx-detect-compilers)
include(setup-build-env)
include(versions-gen)


#
# SUB-xxx
#


add_subdirectory(libs)
add_subdirectory(src)
if (ENABLE_TESTS)
    add_subdirectory(tests)
endif()
add_subdirectory(docs)
attach_doxygen_to(hicc)

#

#
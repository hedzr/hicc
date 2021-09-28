## Get the directory path of the <target>.cmake file
## get_filename_component(CMDR11_CMAKE_DIR "cmdr11Config.cmake" DIRECTORY)
get_filename_component(HICC_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(HICC_INCLUDE_DIR "${HICC_CMAKE_DIR}/../../../include" REALPATH)

## Add the dependencies of our library
#include(CMakeFindDependencyMacro)
##find_dependency(ZLIB REQUIRED)
##find_dependency(fmt REQUIRED)


# Compute paths
#get_filename_component(HICC_CXX_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
#set(HICC_INCLUDE_DIR "${HICC_CMAKE_DIR}/../../../include")


## Import the targets
if (NOT TARGET hicc::hicc)
  #include("${SM_LIB_CMAKE_DIR}/hiccTargets.cmake")
  include("${CMAKE_CURRENT_LIST_DIR}/hiccTargets.cmake")

  # These are IMPORTED targets created by hiccTargets.cmake
  set(HICC_LIBRARIES "hicc::hicc")

  if (EXISTS ${HICC_INCLUDE_DIR}/hicc/hicc.hh)
    set(HICC_FOUND ON)

    set(HICC_INCLUDE_DIRS ${HICC_INCLUDE_DIR})
    get_filename_component(HICC_LIBRARY_DIR "${HICC_CMAKE_DIR}/../../../lib" REALPATH)
    set(HICC_LIBRARY_DIRS ${HICC_LIBRARY_DIR})

    file(READ "${HICC_INCLUDE_DIR}/hicc/hicc-version.hh" ver)

    string(REGEX MATCH "MAJOR_VERSION ([0-9]*)" _ ${ver})
    set(HICC_VERSION_MAJOR ${CMAKE_MATCH_1})

    string(REGEX MATCH "MINOR_VERSION ([0-9]*)" _ ${ver})
    set(HICC_VERSION_MINOR ${CMAKE_MATCH_1})

    string(REGEX MATCH "PATCH_NUMBER ([0-9]*)" _ ${ver})
    set(HICC_VERSION_PATCH ${CMAKE_MATCH_1})

    string(REGEX MATCH "HICC_VERSION_STRING xT\\(\\\"([0-9.]+)\\\"\\)" _ ${ver})
    set(HICC_VERSION_STRING ${CMAKE_MATCH_1})

    string(REGEX MATCH "HICC_GIT_COMMIT_HASH xT\\(\\\"([0-9a-f.-]+)\\\"\\)" _ ${ver})
    set(HICC_VERSION_GIT_HASH ${CMAKE_MATCH_1})
    
    message(">=< HICC_VERSION        = ${HICC_VERSION_MAJOR}.${HICC_VERSION_MINOR}.${HICC_VERSION_PATCH}")
    message(">=< HICC_VERSION_STRING = ${HICC_VERSION_STRING} (HASH: ${HICC_VERSION_GIT_HASH})")
    message(">=< HICC_INCLUDE_DIR    = ${HICC_INCLUDE_DIR}")
    message(">=< HICC_LIBRARY_DIR    = ${HICC_LIBRARY_DIR}")
    message(">=< HICC_LIBRARIES      = ${HICC_LIBRARIES}")
  endif ()

endif ()

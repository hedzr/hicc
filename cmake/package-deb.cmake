set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

# https://cmake.org/cmake/help/latest/module/CPack.html
# https://cmake.org/cmake/help/latest/cpack_gen/deb.html#cpack_gen:CPack%20DEB%20Generator

set(CPACK_DEBIAN_PACKAGE_NAME "libhicc-dev")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})

#set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})

#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.4), libgcc-s1 (>= 3.0), libstdc++6 (>= 9)")
##set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.3.1-6), libc6 (<< 3)")
##set(CPACK_DEBIAN_PACKAGE_DEPENDS "cssrobopec,libqt4-xml,libqt4-network,libqtgui4,treeupdatablereeti")
#set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Debian/postinst")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

#set(CPACK_DEBIAN_PACKAGE_LICENSE ${CPACK_RESOURCE_FILE_LICENSE})
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${CPACK_PACKAGE_HOMEPAGE})


if (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
    set(CPU_ARCH "x64" CACHE STRING "ARCH x86_64" FORCE)
    #set(CPU_ARCH_NAME "x86_64" CACHE STRING "ARCH x86_64" FORCE)
    set(CPU_ARCH_NAME "amd64" CACHE STRING "ARCH x86_64" FORCE)
else ()
    set(CPU_ARCH "x86" CACHE STRING "ARCH x86" FORCE)
    set(CPU_ARCH_NAME "x86" CACHE STRING "ARCH x86_64" FORCE)
endif ()

set(CPACK_TARGET_FILE_NAME "${PROJECT_NAME}_${PROJECT_VERSION}_${CPU_ARCH_NAME}.deb")
#set(CPACK_TARGET_FILE_NAME "${CPACK_DEBIAN_FILE_NAME}")
message(STATUS "CPACK_DEB_FILE_NAME = ${CPACK_DEB_FILE_NAME}")

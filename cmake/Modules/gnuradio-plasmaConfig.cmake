find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_PLASMA gnuradio-plasma)

FIND_PATH(
    GR_PLASMA_INCLUDE_DIRS
    NAMES gnuradio/plasma/api.h
    HINTS $ENV{PLASMA_DIR}/include
        ${PC_PLASMA_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_PLASMA_LIBRARIES
    NAMES gnuradio-plasma
    HINTS $ENV{PLASMA_DIR}/lib
        ${PC_PLASMA_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-plasmaTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_PLASMA DEFAULT_MSG GR_PLASMA_LIBRARIES GR_PLASMA_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_PLASMA_LIBRARIES GR_PLASMA_INCLUDE_DIRS)

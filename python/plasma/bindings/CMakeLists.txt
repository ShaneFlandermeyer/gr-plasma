# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT plasma_sources)
    MESSAGE(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT plasma_sources)

########################################################################
# Check for pygccxml
########################################################################
GR_PYTHON_CHECK_MODULE_RAW(
    "pygccxml"
    "import pygccxml"
    PYGCCXML_FOUND
    )

include(GrPybind)

########################################################################
# Python Bindings
########################################################################
message(STATUS "Extra includes: ${Qt5Widgets_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS} ")
set(extra_includes ${Qt5Widgets_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS})
get_directory_property( DirDefs COMPILE_DEFINITIONS )
message(STATUS "COMPILE_DEFINITIONS = ${DirDefs}" )
set(define_symbols ${DirDefs};__PIC__)
message(STATUS "OLd defines: $CACHE{OLD_DEFINES}" )
message(STATUS "Definitions = ${define_symbols}" )
string(COMPARE NOTEQUAL "$CACHE{OLD_DEFINES}" "${define_symbols}" FORCE_REBUILD )
set(OLD_DEFINES "${define_symbols}" CACHE STRING "Save old defines" FORCE)
message(STATUS "Bindings rebuild forced ${FORCE_REBUILD}")

list(APPEND plasma_python_files
    lfm_source_python.cc
    waveform_controller_python.cc
    usrp_radar_python.cc
    pdu_file_sink_python.cc
    pdu_head_python.cc
    pcfm_source_python.cc
    python_bindings.cc
    range_doppler_sink_python.cc
    match_filt_python.cc
    doppler_processing_python.cc
)

GR_PYBIND_MAKE_OOT(plasma
   ../../..
   gr::plasma
   "${plasma_python_files}")

# copy in bindings .so file for use in QA test module
add_custom_target(
  copy_bindings_for_tests ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/lib/*.so"
    ${CMAKE_BINARY_DIR}/test_modules/gnuradio/plasma/
  DEPENDS plasma_python)

install(TARGETS plasma_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/plasma COMPONENT pythonapi)

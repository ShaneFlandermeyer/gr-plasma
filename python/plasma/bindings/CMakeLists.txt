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

list(APPEND plasma_python_files
    python_bindings.cc)

GR_PYBIND_MAKE_OOT(plasma
   ../../..
   gr::plasma
   "${plasma_python_files}")

# copy in bindings .so file for use in QA test module
add_custom_target(
  copy_bindings_for_tests ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/*.so"
    ${CMAKE_BINARY_DIR}/test_modules/gnuradio/plasma/
  DEPENDS plasma_python)

install(TARGETS plasma_python DESTINATION ${GR_PYTHON_DIR}/gnuradio/plasma COMPONENT pythonapi)

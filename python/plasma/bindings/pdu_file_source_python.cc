/*
 * Copyright 2022 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(pdu_file_source.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(e1f85c2a1b2ee49c668f7e78a90531a5)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/plasma/pdu_file_source.h>
// pydoc.h is automatically generated in the build directory
#include <pdu_file_source_pydoc.h>

void bind_pdu_file_source(py::module& m)
{

    using pdu_file_source = ::gr::plasma::pdu_file_source;


    py::class_<pdu_file_source,
               gr::block,
               gr::basic_block,
               std::shared_ptr<pdu_file_source>>(m, "pdu_file_source", D(pdu_file_source))

        .def(py::init(&pdu_file_source::make),
             py::arg("data_filename"),
             py::arg("meta_filename"),
             py::arg("offset"),
             py::arg("length"),
             D(pdu_file_source, make))


        ;
}

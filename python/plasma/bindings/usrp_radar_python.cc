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
/* BINDTOOL_GEN_AUTOMATIC(1)                                                       */
/* BINDTOOL_USE_PYGCCXML(1)                                                        */
/* BINDTOOL_HEADER_FILE(usrp_radar.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(65614bc695ecc58c090c911fce3bd125)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/plasma/usrp_radar.h>
// pydoc.h is automatically generated in the build directory
#include <usrp_radar_pydoc.h>

void bind_usrp_radar(py::module& m)
{

    using usrp_radar = ::gr::plasma::usrp_radar;


    py::class_<usrp_radar, gr::block, gr::basic_block, std::shared_ptr<usrp_radar>>(
        m, "usrp_radar", D(usrp_radar))

        .def(py::init(&usrp_radar::make),
             py::arg("samp_rate"),
             py::arg("tx_gain"),
             py::arg("rx_gain"),
             py::arg("tx_freq"),
             py::arg("rx_freq"),
             py::arg("start_time"),
             py::arg("tx_args"),
             py::arg("rx_args"),
             D(usrp_radar, make))


        .def("set_tx_thread_priority",
             &usrp_radar::set_tx_thread_priority,
             py::arg("arg0"),
             D(usrp_radar, set_tx_thread_priority))


        .def("set_rx_thread_priority",
             &usrp_radar::set_rx_thread_priority,
             py::arg("arg0"),
             D(usrp_radar, set_rx_thread_priority))

        ;
}
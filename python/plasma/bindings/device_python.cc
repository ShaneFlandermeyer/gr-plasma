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
/* BINDTOOL_HEADER_FILE(device.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(9ee386279f14c5616fe6087bfbe1b8ca)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/plasma/device.h>
// pydoc.h is automatically generated in the build directory
#include <device_pydoc.h>

void bind_device(py::module& m)
{

    using Device = ::gr::plasma::Device;


    py::class_<Device, std::shared_ptr<Device>> device_class(m, "Device", D(Device));

    device_class.def(py::init<>(), D(Device, Device, 0));
    device_class.def(py::init<gr::plasma::Device const&>(), py::arg("arg0"), D(Device, Device, 1));

    py::enum_<gr::plasma::Device::Backend>(device_class, "Backend")
        .value("DEFAULT", gr::plasma::Device::DEFAULT)
        .value("CPU", gr::plasma::Device::CPU)
        .value("CUDA", gr::plasma::Device::CUDA)
        .value("OPENCL", gr::plasma::Device::OPENCL) // 999
        .export_values();
    py::implicitly_convertible<int, gr::plasma::Device::Backend>();
}
/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <pybind11/pybind11.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Headers for binding functions
/**************************************/
// The following comment block is used for
// gr_modtool to insert function prototypes
// Please do not delete
/**************************************/
// BINDING_FUNCTION_PROTOTYPES(
    void bind_lfm_source(py::module& m);
    void bind_waveform_controller(py::module& m);
    void bind_usrp_radar(py::module& m);
    void bind_pdu_file_sink(py::module& m);
    void bind_pdu_head(py::module& m);
    void bind_pcfm_source(py::module& m);
    void bind_range_doppler_sink(py::module& m);
    void bind_match_filt(py::module& m);
    void bind_doppler_processing(py::module& m);
// ) END BINDING_FUNCTION_PROTOTYPES


// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
void* init_numpy()
{
    import_array();
    return NULL;
}

PYBIND11_MODULE(plasma_python, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    // Allow access to base block methods
    py::module::import("gnuradio.gr");
    py::module::import("gnuradio.qtgui.qtgui_python");
    /**************************************/
    // The following comment block is used for
    // gr_modtool to insert binding function calls
    // Please do not delete
    /**************************************/
    // BINDING_FUNCTION_CALLS(
    bind_lfm_source(m);
    bind_waveform_controller(m);
    bind_usrp_radar(m);
    bind_pdu_file_sink(m);
    bind_pdu_head(m);
    bind_pcfm_source(m);
    bind_range_doppler_sink(m);
    bind_match_filt(m);
    bind_doppler_processing(m);
    // ) END BINDING_FUNCTION_CALLS
}
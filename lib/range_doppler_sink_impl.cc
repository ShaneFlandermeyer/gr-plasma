/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "range_doppler_sink_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

// #pragma message("set the following appropriately and remove this warning")
// using input_type = float;
// #pragma message("set the following appropriately and remove this warning")
// using output_type = float;
range_doppler_sink::sptr range_doppler_sink::make(QWidget* parent)
{
    return gnuradio::make_block_sptr<range_doppler_sink_impl>(parent);
}


/*
 * The private constructor
 */
range_doppler_sink_impl::range_doppler_sink_impl(QWidget* parent)
    : gr::block("range_doppler_sink",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0))
{
    // Initialize the QApplication
    d_argc = 1;
    d_argv = new char;
    d_argv[0] = '\0';
    if (qApp != NULL)
        d_qapp = qApp;
    else
        d_qapp = new QApplication(d_argc, &d_argv);

    // TODO: Substitute a range doppler QWidget
    widget = new QWidget();

    // TODO: Add a function to handle input PDUs
    message_port_register_in(pmt::mp("pdu"));
    GR_LOG_DEBUG(d_logger, "End of constructor reached!");
}

/*
 * Our virtual destructor.
 */
range_doppler_sink_impl::~range_doppler_sink_impl() {}

void range_doppler_sink_impl::exec_() { d_qapp->exec(); };

QWidget* range_doppler_sink_impl::qwidget() { return (QWidget*)widget; }

#ifdef ENABLE_PYTHON
PyObject* range_doppler_sink_impl::pyqwidget()
{
    PyObject* w = PyLong_FromVoidPtr((void*)widget);
    PyObject* retarg = Py_BuildValue("N", w);
    return retarg;
}
#else
void* range_doppler_sink_impl::pyqwidget() { return nullptr; }
#endif


} /* namespace plasma */
} /* namespace gr */

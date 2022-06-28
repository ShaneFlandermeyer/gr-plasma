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
    d_win = new Window(parent);
    for (int index = 0; index < d_win->plotDataSize; ++index) {
        d_win->xData[index] = index;
        d_win->yData[index] = 0;
    }

    // TODO: Add a function to handle input PDUs
    message_port_register_in(pmt::mp("pdu"));
    set_msg_handler(pmt::mp("pdu"), [this](pmt::pmt_t msg) { handle_pdu(msg); });
    GR_LOG_DEBUG(d_logger, "End of constructor reached!");
}

/*
 * Our virtual destructor.
 */
range_doppler_sink_impl::~range_doppler_sink_impl()
{
    delete d_argv;
    if (not d_win->isClosed())
        d_win->close();
}

void range_doppler_sink_impl::exec_() { d_qapp->exec(); };

QWidget* range_doppler_sink_impl::qwidget() { return (QWidget*)d_win; }

#ifdef ENABLE_PYTHON
PyObject* range_doppler_sink_impl::pyqwidget()
{
    PyObject* w = PyLong_FromVoidPtr((void*)d_win);
    PyObject* retarg = Py_BuildValue("N", w);
    return retarg;
}
#else
void* range_doppler_sink_impl::pyqwidget() { return nullptr; }
#endif

void range_doppler_sink_impl::handle_pdu(pmt::pmt_t pdu)
{
    double inVal = sin(M_PI * d_win->count / 50.0);
    ++d_win->count;

    // add the new input to the plot
    std::move(d_win->yData, d_win->yData + d_win->plotDataSize - 1, d_win->yData + 1);
    d_win->yData[0] = inVal;
    d_win->curve->setSamples(d_win->xData, d_win->yData, d_win->plotDataSize);
    d_win->plot->replot();
    d_win->plot->update();

    // set the thermometer value
    d_win->thermo->setValue(fabs(inVal));
}
} /* namespace plasma */
} /* namespace gr */

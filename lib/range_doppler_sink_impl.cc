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
    d_main_gui = new RangeDopplerWindow(parent);

    // d_magbufs = volk::vector<double>();
    // d_residbufs = volk::vector<gr_complex>();

    // TODO: Add a function to handle input PDUs
    message_port_register_in(pmt::mp("tx"));
    message_port_register_in(pmt::mp("rx"));
    set_msg_handler(pmt::mp("tx"), [this](pmt::pmt_t msg) { handle_tx_msg(msg); });
    set_msg_handler(pmt::mp("rx"), [this](pmt::pmt_t msg) { handle_rx_msg(msg); });
}

/*
 * Our virtual destructor.
 */
range_doppler_sink_impl::~range_doppler_sink_impl()
{
    delete d_argv;
    if (not d_main_gui->is_closed())
        d_main_gui->close();
}

void range_doppler_sink_impl::exec_() { d_qapp->exec(); };

QWidget* range_doppler_sink_impl::qwidget() { return (QWidget*)d_main_gui; }

#ifdef ENABLE_PYTHON
PyObject* range_doppler_sink_impl::pyqwidget()
{
    PyObject* w = PyLong_FromVoidPtr((void*)d_main_gui);
    PyObject* retarg = Py_BuildValue("N", w);
    return retarg;
}
#else
void* range_doppler_sink_impl::pyqwidget() { return nullptr; }
#endif

void range_doppler_sink_impl::handle_tx_msg(pmt::pmt_t msg)
{
    GR_LOG_DEBUG(d_logger, "Tx message received");
    if (pmt::is_pdu(msg)) {
        // Get the transmit data
        pmt::pmt_t samples = pmt::cdr(msg);
        size_t n = pmt::length(samples);
        gr_complex* in = (gr_complex*)pmt::c32vector_elements(samples, n);
        Eigen::ArrayXcf invec = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(in, n);
        d_magbufs = invec.real().cast<double>();
        d_qapp->postEvent(d_main_gui, new RangeDopplerUpdateEvent(d_magbufs));
    }
}

void range_doppler_sink_impl::handle_rx_msg(pmt::pmt_t msg)
{
    // GR_LOG_DEBUG(d_logger, "Rx message received");
    // TODO: Pass the data to the GUI via a QEvent subclass object
    // d_qapp->postEvent(d_main_gui, new QEvent(QEvent::Type(10000)));
}
} /* namespace plasma */
} /* namespace gr */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "range_doppler_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <chrono>
#include <thread>

namespace gr {
namespace plasma {


range_doppler_sink::sptr range_doppler_sink::make(double samp_rate,
                                                  size_t num_pulse_cpi,
                                                  double center_freq,
                                                  QWidget* parent)
{
    return gnuradio::make_block_sptr<range_doppler_sink_impl>(
        samp_rate, num_pulse_cpi, center_freq, parent);
}


/*
 * The private constructor
 */
range_doppler_sink_impl::range_doppler_sink_impl(double samp_rate,
                                                 size_t num_pulse_cpi,
                                                 double center_freq,
                                                 QWidget* parent)
    : gr::block("range_doppler_sink",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_samp_rate(samp_rate),
      d_num_pulse_cpi(num_pulse_cpi),
      d_center_freq(center_freq)

{
    // Initialize the QApplication
    d_argc = 1;
    d_argv = new char;
    d_argv[0] = '\0';
    if (qApp != NULL)
        d_qapp = qApp;
    else
        d_qapp = new QApplication(d_argc, &d_argv);
    d_main_gui = new RangeDopplerWindow(parent);

    // Initialize message ports
    d_in_port = pmt::intern("in");
    message_port_register_in(d_in_port);
    set_msg_handler(d_in_port, [this](pmt::pmt_t msg) { handle_rx_msg(msg); });
}

/*
 * Our virtual destructor.
 */
range_doppler_sink_impl::~range_doppler_sink_impl()
{
    delete d_argv;
}

bool range_doppler_sink_impl::start()
{
    d_finished = false;
    return block::start();
}

bool range_doppler_sink_impl::stop()
{

    d_finished = true;
    if (not d_main_gui->is_closed())
        d_main_gui->close();
    return block::stop();
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

void range_doppler_sink_impl::handle_rx_msg(pmt::pmt_t msg)
{
    if (d_finished) {
        return;
    }
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
        d_meta = pmt::car(msg);
    }
    size_t n = pmt::length(samples);
    Eigen::Map<Eigen::ArrayXXcf> in(pmt::c32vector_writable_elements(samples, n),
                                     n / d_num_pulse_cpi,
                                     d_num_pulse_cpi);
    // Eigen::ArrayXXd newdata = Eigen::ArrayXXd::Ones(100,25);
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> plot_data =
        20 * log10(abs(in)).cast<double>();
    // Normalize the plot data and clip it to fit the dynamic range
    plot_data = plot_data - plot_data.maxCoeff();
    plot_data = plot_data.max(-d_dynamic_range_db);


    d_qapp->postEvent(d_main_gui,
                      new RangeDopplerUpdateEvent(
                          plot_data.data(), plot_data.rows(), plot_data.cols(), d_meta));
}


void range_doppler_sink_impl::set_dynamic_range(const double r)
{
    d_dynamic_range_db = r;
}

void range_doppler_sink_impl::set_msg_queue_depth(size_t depth)
{
    d_msg_queue_depth = depth;
}

} /* namespace plasma */
} /* namespace gr */
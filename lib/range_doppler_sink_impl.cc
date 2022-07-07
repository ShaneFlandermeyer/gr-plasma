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
    // GR_LOG_DEBUG(d_logger, "Destroying")
    // delete d_argv;
    // delete d_main_gui;
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

// void range_doppler_sink_impl::fftresize(size_t size)
// {
//     gr::thread::scoped_lock lock(d_setlock);
//     // Make the fft a power of 2
//     size_t newsize = pow(2, log(size) / log(2));

//     if (newsize != d_fftsize) {
//         d_fftsize = size;

//         d_conv_fwd = std::make_unique<fft::fft_complex_fwd>(newsize);
//         d_doppler_fft = std::make_unique<fft::fft_complex_fwd>(d_num_pulse_cpi);
//         d_conv_inv = std::make_unique<fft::fft_complex_rev>(newsize);
//         // Set the number of threads used for the FFT computations
//         d_conv_fwd->set_nthreads(d_num_fft_thread);
//         d_conv_inv->set_nthreads(d_num_fft_thread);
//         d_doppler_fft->set_nthreads(d_num_fft_thread);


//         d_shift.resize(d_num_pulse_cpi);

//         // Pre-compute the frequency-domain matched filter
//         memcpy(d_conv_fwd->get_inbuf(),
//                d_matched_filter.data(),
//                d_matched_filter.size() * sizeof(gr_complex));
//         for (size_t i = d_matched_filter.size(); i < d_fftsize; i++)
//             d_conv_fwd->get_inbuf()[i] = 0;
//         d_conv_fwd->execute();
//         d_matched_filter_freq = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(
//             d_conv_fwd->get_outbuf(), newsize);
//         // Update the axes
//         double prf = d_samp_rate / (double)(size - d_matched_filter.size() + 1);
//         double c = ::plasma::physconst::c;
//         double lam = c / d_center_freq;
//         double vmax = (lam / 2) * (prf / 2);
//         double rmin = (c / 2) * -d_matched_filter.size() / d_samp_rate;
//         double rmax = (c / 2) * (size - d_matched_filter.size()) / d_samp_rate;
//         d_main_gui->xlim(-vmax, vmax);
//         d_main_gui->ylim(rmin, rmax);
//     }
// }

// void range_doppler_sink_impl::handle_tx_msg(pmt::pmt_t msg)
// {
//     if (pmt::is_pdu(msg)) {
//         // Get the transmit data
//         pmt::pmt_t samples = pmt::cdr(msg);
//         size_t n = pmt::length(samples);
//         std::vector<gr_complex> data = pmt::c32vector_elements(samples);
//         d_matched_filter = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(data.data(), n);
//         d_matched_filter = d_matched_filter.conjugate().reverse();
//     }
// }

void range_doppler_sink_impl::handle_rx_msg(pmt::pmt_t msg)
{
    if (d_finished or this->nmsgs(d_in_port) > d_msg_queue_depth) {
        return;
    }
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
    }
    size_t n = pmt::length(samples);
    Eigen::Map<Eigen::ArrayXXcf> in(pmt::c32vector_writable_elements(samples, n),
                                    n / d_num_pulse_cpi,
                                    d_num_pulse_cpi);

    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> plot_data =
        20 * log10(abs(in)).cast<double>();
    // Normalize the plot data and clip it to fit the dynamic range
    plot_data = plot_data - plot_data.maxCoeff();
    for (int i = 0; i < plot_data.size(); i++) {
        if (plot_data.data()[i] < -d_dynamic_range_db)
            plot_data.data()[i] = -d_dynamic_range_db;
    }
    d_qapp->postEvent(d_main_gui,
                      new RangeDopplerUpdateEvent(
                          plot_data.data(), plot_data.rows(), plot_data.cols()));
    // TODO: Update the range and doppler axes
    d_main_gui->xlim(0, in.cols());
    d_main_gui->ylim(0, in.rows());
    //         double prf = d_samp_rate / (double)(size - d_matched_filter.size() + 1);
    //         double c = ::plasma::physconst::c;
    //         double lam = c / d_center_freq;
    //         double vmax = (lam / 2) * (prf / 2);
    //         double rmin = (c / 2) * -d_matched_filter.size() / d_samp_rate;
    //         double rmax = (c / 2) * (size - d_matched_filter.size()) / d_samp_rate;
    //         d_main_gui->xlim(-vmax, vmax);
    //         d_main_gui->ylim(rmin, rmax);
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
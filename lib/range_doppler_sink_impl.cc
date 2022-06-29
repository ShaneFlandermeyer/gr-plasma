/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "range_doppler_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <thread>

namespace gr {
namespace plasma {

range_doppler_sink::sptr
range_doppler_sink::make(double samp_rate, size_t num_pulse_cpi, QWidget* parent)
{
    return gnuradio::make_block_sptr<range_doppler_sink_impl>(
        samp_rate, num_pulse_cpi, parent);
}


/*
 * The private constructor
 */
range_doppler_sink_impl::range_doppler_sink_impl(double samp_rate,
                                                 size_t num_pulse_cpi,
                                                 QWidget* parent)
    : gr::block("range_doppler_sink",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_samp_rate(samp_rate),
      d_num_pulse_cpi(num_pulse_cpi),
      d_shift(1)

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

    d_count = 0;

    // Initialize message ports
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
        gr_complex* data = pmt::c32vector_writable_elements(samples, n);
        d_matched_filter = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(data, n);
        d_matched_filter = d_matched_filter.conjugate().reverse();
    }
}

void range_doppler_sink_impl::handle_rx_msg(pmt::pmt_t msg)
{
    GR_LOG_DEBUG(d_logger, "Rx message received");
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t samples = pmt::cdr(msg);
        size_t n = pmt::length(samples);
        gr_complex* data = pmt::c32vector_writable_elements(samples, n);
        if (d_fast_time_slow_time.size() == 0) {
            // Initialize the fast-time/slow-time data matrix if it hasn't
            // already been done
            // TODO: This currently assumes a uniform PRF
            d_fast_time_slow_time = Eigen::ArrayXXcf(n, d_num_pulse_cpi);
        }
        d_fast_time_slow_time.col(d_count) =
            Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(data, n);
        // TODO: Used for debugging convolution. Remove this and use the tx port
        d_matched_filter = d_fast_time_slow_time.col(d_count).reverse().conjugate();
        d_count++;

        if (d_matched_filter.size() == 0) {
            GR_LOG_DEBUG(d_logger, "Matched filter not yet received")
        } else {
            GR_LOG_DEBUG(d_logger, "Performing matched filter operation")
            // Do a matched filter operation for the current pulse
            // Initialize the FFT objects
            int fftsize = n + d_matched_filter.size() - 1;

            // TODO: Move this somewhere else
            d_range_slow_time = Eigen::ArrayXXcf(fftsize, d_num_pulse_cpi);

            // d_matched_filter = d_matched_filter * scale_factor;
            d_conv_fwd = std::make_unique<fft::fft_complex_fwd>(fftsize);
            d_doppler_fft = std::make_unique<fft::fft_complex_fwd>(d_num_pulse_cpi);
            d_conv_inv = std::make_unique<fft::fft_complex_rev>(fftsize);

            // Zero-pad the input and transform
            memcpy(d_conv_fwd->get_inbuf(),
                   d_fast_time_slow_time.col(d_count - 1).data(),
                   d_fast_time_slow_time.rows() * sizeof(gr_complex));
            d_conv_fwd->execute();
            for (auto i = d_fast_time_slow_time.size(); i < d_conv_fwd->inbuf_length();
                 i++) {
                d_conv_fwd->get_inbuf()[i] = 0;
            }
            gr_complex* out = d_conv_fwd->get_outbuf();
            gr_complex* a = new gr_complex[fftsize];
            memcpy(a, out, fftsize * sizeof(gr_complex));

            // Zero-pad the matched filter and transform
            memcpy(d_conv_fwd->get_inbuf(),
                   d_matched_filter.data(),
                   d_matched_filter.size() * sizeof(gr_complex));
            for (auto i = d_matched_filter.size(); i < d_conv_fwd->inbuf_length(); i++) {
                d_conv_fwd->get_inbuf()[i] = 0;
            }
            d_conv_fwd->execute();
            gr_complex* b = d_conv_fwd->get_outbuf();

            // Set the IFFT input to the result of an element-wise
            // multiplication in the frequency domain
            gr_complex* c = d_conv_inv->get_inbuf();
            volk_32fc_x2_multiply_32fc_a(c, a, b, fftsize);
            d_conv_inv->execute();

            Eigen::ArrayXcf d = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(
                d_conv_inv->get_outbuf(), fftsize);
            d *= 1 / (float)fftsize;

            // TODO: Used for debugging range doppler plot. Remove
            for (size_t i = 0; i < d_num_pulse_cpi; i++) {
                d_range_slow_time.col(i) = d;
            }
            // Do a slow-time FFT to get a range-doppler map
            d_range_doppler = Eigen::ArrayXXcf(fftsize, d_num_pulse_cpi);
            for (auto i = 0; i < fftsize; i++) {
                Eigen::ArrayXcf tmp = d_range_slow_time.row(i);
                memcpy(d_doppler_fft->get_inbuf(),
                       tmp.data(),
                       tmp.size() * sizeof(gr_complex));
                d_doppler_fft->execute();
                d_shift.shift(d_doppler_fft->get_outbuf(),
                              d_doppler_fft->outbuf_length());
                d_range_doppler.row(i) = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(
                    d_doppler_fft->get_outbuf(), d_doppler_fft->outbuf_length());
            }
        }

        // Convert the data to a form that can be plotted and send it to the GUI
        Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> plot_data =
            abs(d_range_doppler).cast<double>();
        d_qapp->postEvent(d_main_gui,
                          new RangeDopplerUpdateEvent(
                              plot_data.data(), plot_data.rows(), plot_data.cols()));
    }
}
} /* namespace plasma */
} /* namespace gr */

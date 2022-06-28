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
    d_main_gui = new RangeDopplerWindow(parent);

    // d_filter = filter::kernel::fft_filter_ccc(1, std::vector<gr_complex>());


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
        std::vector<gr_complex> taps = pmt::c32vector_elements(samples);

        // Store the frequency domain matched filter data
        d_fwd = std::make_unique<fft::fft_complex_fwd>(n);
        // d_inv = std::make_unique<fft::fft_complex_rev>(n);
        d_xformed_taps.resize(n);
        gr_complex* in = d_fwd->get_inbuf();
        gr_complex* out = d_fwd->get_outbuf();
        float scale = 1.0 / n;
        size_t i = 0;
        for (i = 0; i < taps.size(); i++) {
            in[i] = taps[i] * scale;
        }
        for (; i < n; i++)
            in[i] = 0;
        d_fwd->execute();
        for (i = 0; i < n; i++)
            d_xformed_taps[i] = out[i];

        // Get the data that will be used for plotting
        d_magbuf = Eigen::ArrayXd(n);
        for (size_t i = 0; i < n; i++) {
            d_magbuf[i] = 20 * log10(abs(d_xformed_taps[i]));
        }

        d_qapp->postEvent(d_main_gui,
                          new RangeDopplerUpdateEvent(d_magbuf, d_magbuf.size()));
    }
}

// void range_doppler_sink_impl::fft(float* data_out, const gr_complex* data_in, int size)
// {
//     d_fft = std::make_unique<fft::fft_complex_fwd>(size);
//     memcpy(d_fft->get_inbuf(), data_in, sizeof(gr_complex) * size);

//     d_fft->execute(); // compute the fft

//     volk_32fc_s32f_x2_power_spectral_density_32f(
//         data_out, d_fft->get_outbuf(), size, 1.0, size);

//     d_fft_shift.shift(data_out, size);
// }

void range_doppler_sink_impl::handle_rx_msg(pmt::pmt_t msg)
{
    GR_LOG_DEBUG(d_logger, "Rx message received");
    // // TODO: Pass the data to the GUI via a QEvent subclass object
    // // d_qapp->postEvent(d_main_gui, new QEvent(QEvent::Type(10000)));
    // while (d_xformed_taps.size() == 0) {
    //     std::this_thread::sleep_for(std::chrono::microseconds(100));
    // }
    // if (pmt::is_pdu(msg)) {
    //     pmt::pmt_t samples = pmt::cdr(msg);
    //     int n = pmt::length(samples);
    //     int fftsize = n + d_xformed_taps.size() - 1;
    //     if (fftsize != d_fwd->inbuf_length()) {
    //         GR_LOG_DEBUG(d_logger, "Resizing");
    //         d_fwd = std::make_unique<fft::fft_complex_fwd>(fftsize);
    //         d_inv = std::make_unique<fft::fft_complex_rev>(fftsize);
    //     }

    //     // vol
    // }
}
} /* namespace plasma */
} /* namespace gr */

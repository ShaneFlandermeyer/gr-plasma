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

// TODO: Delete
using namespace std::chrono;
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
      d_center_freq(center_freq),
      d_shift(num_pulse_cpi)

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

    // TODO: Make this a block parameter
    d_num_fft_thread = 10;
    d_dynamic_range_db = 60;

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
    d_processing_thread.join();
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

void range_doppler_sink_impl::fftresize(size_t size)
{
    gr::thread::scoped_lock lock(d_setlock);

    if (size != d_fftsize) {
        d_fftsize = size;

        d_conv_fwd = std::make_unique<fft::fft_complex_fwd>(size);
        d_doppler_fft = std::make_unique<fft::fft_complex_fwd>(d_num_pulse_cpi);
        d_conv_inv = std::make_unique<fft::fft_complex_rev>(size);
        // Set the number of threads used for the FFT computations
        d_conv_fwd->set_nthreads(d_num_fft_thread);
        d_conv_inv->set_nthreads(d_num_fft_thread);
        d_doppler_fft->set_nthreads(d_num_fft_thread);


        d_shift.resize(d_num_pulse_cpi);

        d_range_slow_time = Eigen::ArrayXXcf(size, d_num_pulse_cpi);
        d_range_doppler = Eigen::ArrayXXcf(size, d_num_pulse_cpi);

        // Pre-compute the frequency-domain matched filter
        memcpy(d_conv_fwd->get_inbuf(),
               d_matched_filter.data(),
               d_matched_filter.size() * sizeof(gr_complex));
        for (auto i = d_matched_filter.size(); i < d_conv_fwd->inbuf_length(); i++)
            d_conv_fwd->get_inbuf()[i] = 0;
        d_conv_fwd->execute();
        gr_complex* b = d_conv_fwd->get_outbuf();
        d_matched_filter_freq = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(b, size);
        // Update the axes
        double prf = d_samp_rate / (double)(size - d_matched_filter.size() + 1);
        double c = ::plasma::physconst::c;
        double lam = c / d_center_freq;
        double vmax = (lam / 2) * (prf / 2);
        d_main_gui->xlim(-vmax, vmax);
        double rmin = (c / 2) * -d_matched_filter.size() / d_samp_rate;
        double rmax = (c / 2) * (size - d_matched_filter.size()) / d_samp_rate;
        d_main_gui->ylim(rmin, rmax);
    }
}

void range_doppler_sink_impl::handle_tx_msg(pmt::pmt_t msg)
{
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
    if (d_finished or d_matched_filter.size() == 0) {
        // GR_LOG_INFO(d_logger, "Matched filter PDU not yet received");
        return;
    }
    // GR_LOG_DEBUG(d_logger, "Rx message received");
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
    }
    size_t n = pmt::length(samples);
    gr_complex* in = pmt::c32vector_writable_elements(samples, n);
    if (d_fast_time_slow_time.size() == 0) {
        // Initialize the fast-time/slow-time data matrix if it hasn't
        // already been done
        // TODO: This currently assumes a uniform PRF
        d_fast_time_slow_time = Eigen::ArrayXXcf(n, d_num_pulse_cpi);
    }
    d_fast_time_slow_time.col(d_count) =
        Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(in, n);

    if (d_count < d_num_pulse_cpi - 1) {
        d_count++;
    } else {
        d_processing_thread.join();
        d_processing_thread = gr::thread::thread([this] { process_data(); });
        d_count = 0;
    }
}

void range_doppler_sink_impl::process_data()
{
    auto nfft = d_fast_time_slow_time.rows() + d_matched_filter.size() - 1;
    for (int i = 0; i < d_fast_time_slow_time.cols(); i++) {
        gr_complex* mfresp = conv(d_fast_time_slow_time.col(i).data(),
                                  d_matched_filter.data(),
                                  d_fast_time_slow_time.rows(),
                                  d_matched_filter.size());
        // d_range_slow_time.col(i) =
        //     Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(mfresp, nfft);
        memcpy(d_range_slow_time.col(i).data(), mfresp, nfft * sizeof(gr_complex));
        delete[] mfresp;
    }
    d_range_slow_time *= 1 / (float)nfft;

    // Do doppler processing
    for (auto i = 0; i < d_range_slow_time.rows(); i++) {
        Eigen::ArrayXcf tmp = d_range_slow_time.row(i);
        memcpy(d_doppler_fft->get_inbuf(), tmp.data(), tmp.size() * sizeof(gr_complex));
        d_doppler_fft->execute();
        d_shift.shift(d_doppler_fft->get_outbuf(), d_doppler_fft->outbuf_length());
        d_range_doppler.row(i) = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(
            d_doppler_fft->get_outbuf(), d_doppler_fft->outbuf_length());
    }


    // Convert the data to a form that can be plotted and send it to the GUI
    Eigen::Array<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> plot_data =
        20 * log10(abs(d_range_doppler)).cast<double>();
    // Normalize the plot data and clip it to fit the dynamic range
    plot_data = plot_data - plot_data.maxCoeff();
    for (int i = 0; i < plot_data.size(); i++) {
        if (plot_data.data()[i] < -d_dynamic_range_db)
            plot_data.data()[i] = -d_dynamic_range_db;
    }
    d_qapp->postEvent(d_main_gui,
                      new RangeDopplerUpdateEvent(
                          plot_data.data(), plot_data.rows(), plot_data.cols()));
}


void range_doppler_sink_impl::conv()
{
    int fftsize = d_fast_time_slow_time.rows() + d_matched_filter.size() - 1;
    fftresize(fftsize);
    // Zero-pad the input and transform
    memcpy(d_conv_fwd->get_inbuf(),
           d_fast_time_slow_time.col(d_count).data(),
           d_fast_time_slow_time.rows() * sizeof(gr_complex));
    d_conv_fwd->execute();
    for (auto i = d_fast_time_slow_time.size(); i < d_conv_fwd->inbuf_length(); i++)
        d_conv_fwd->get_inbuf()[i] = 0;
    // gr_complex* out = d_conv_fwd->get_outbuf();
    gr_complex* a = new gr_complex[fftsize];
    memcpy(a, d_conv_fwd->get_outbuf(), fftsize * sizeof(gr_complex));

    // Zero-pad the matched filter and transform
    memcpy(d_conv_fwd->get_inbuf(),
           d_matched_filter.data(),
           d_matched_filter.size() * sizeof(gr_complex));
    for (auto i = d_matched_filter.size(); i < d_conv_fwd->inbuf_length(); i++)
        d_conv_fwd->get_inbuf()[i] = 0;
    d_conv_fwd->execute();
    gr_complex* b = d_conv_fwd->get_outbuf();

    // Hadamard product and IFFT
    gr_complex* c = d_conv_inv->get_inbuf();
    volk_32fc_x2_multiply_32fc_a(c, a, b, fftsize);
    d_conv_inv->execute();

    // Scale the result by the FFT size to make it mathematically correct
    d_range_slow_time.col(d_count) =
        Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(d_conv_inv->get_outbuf(), fftsize);
    d_range_slow_time.col(d_count) *= 1 / (float)fftsize;

    delete[] a;
}

gr_complex* range_doppler_sink_impl::conv(const gr_complex* x,
                                          const gr_complex* h,
                                          size_t nx,
                                          size_t nh)
{
    int fftsize = nx + nh - 1;
    fftresize(fftsize);
    // Zero-pad the input and transform
    memcpy(d_conv_fwd->get_inbuf(), x, nx * sizeof(gr_complex));
    d_conv_fwd->execute();
    for (int i = nx; i < fftsize; i++)
        d_conv_fwd->get_inbuf()[i] = 0;
    // gr_complex* out = d_conv_fwd->get_outbuf();
    gr_complex* a = new gr_complex[fftsize];
    memcpy(a, d_conv_fwd->get_outbuf(), fftsize * sizeof(gr_complex));

    // Zero-pad the matched filter and transform
    memcpy(d_conv_fwd->get_inbuf(), h, nh * sizeof(gr_complex));
    for (int i = nh; i < fftsize; i++)
        d_conv_fwd->get_inbuf()[i] = 0;
    d_conv_fwd->execute();
    gr_complex* b = new gr_complex[fftsize];
    memcpy(b, d_conv_fwd->get_outbuf(), fftsize * sizeof(gr_complex));

    // Hadamard product and IFFT
    volk_32fc_x2_multiply_32fc_a(d_conv_inv->get_inbuf(), a, b, fftsize);
    d_conv_inv->execute();
    gr_complex* out = new gr_complex[fftsize];
    memcpy(out, d_conv_inv->get_outbuf(), fftsize * sizeof(gr_complex));

    delete[] a;
    delete[] b;
    return out;
}

} /* namespace plasma */
} /* namespace gr */

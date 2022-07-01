/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "match_filt_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

// #pragma message("set the following appropriately and remove this warning")
// using input_type = float;
// #pragma message("set the following appropriately and remove this warning")
// using output_type = float;
match_filt::sptr match_filt::make(size_t num_pulse_cpi)
{
    return gnuradio::make_block_sptr<match_filt_impl>(num_pulse_cpi);
}


/*
 * The private constructor
 */
match_filt_impl::match_filt_impl(size_t num_pulse_cpi)
    : gr::block(
          "match_filt", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_num_pulse_cpi(num_pulse_cpi)
{
    d_fftsize = 0;
    d_pulse_count = 0;
    message_port_register_in(pmt::mp("tx"));
    message_port_register_in(pmt::mp("rx"));
    message_port_register_out(pmt::mp("out"));
    set_msg_handler(pmt::mp("tx"), [this](pmt::pmt_t msg) { handle_tx_msg(msg); });
    set_msg_handler(pmt::mp("rx"), [this](pmt::pmt_t msg) { handle_rx_msg(msg); });
}

/*
 * Our virtual destructor.
 */
match_filt_impl::~match_filt_impl() {}

bool match_filt_impl::start()
{
    d_finished = false;
    return block::start();
}

bool match_filt_impl::stop()
{
    d_finished = true;
    d_processing_thread.interrupt();
    return block::stop();
}

void match_filt_impl::handle_tx_msg(pmt::pmt_t msg)
{
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        // Get the transmit data
        samples = pmt::cdr(msg);
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    size_t n = pmt::length(samples);
    std::vector<gr_complex> data = pmt::c32vector_elements(samples);
    d_match_filt = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(data.data(), n);
    d_match_filt = d_match_filt.conjugate().reverse();
}

void match_filt_impl::handle_rx_msg(pmt::pmt_t msg)
{
    if (d_finished or d_match_filt.size() == 0)
        return;
    // Get a copy of the input samples
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    size_t n = pmt::length(samples);
    std::vector<gr_complex> in = pmt::c32vector_elements(samples);
    if (d_fast_slow_time.size() == 0) {
        // Initialize the fast-time/slow-time data matrix if it hasn't
        // already been done
        // TODO: This currently assumes a uniform PRF
        d_fast_slow_time = Eigen::ArrayXXcf(n, d_num_pulse_cpi);
    }
    d_fast_slow_time.col(d_pulse_count) = Eigen::Map<Eigen::ArrayXcf>(in.data(), n);

    if (d_pulse_count < d_num_pulse_cpi - 1) {
        // Haven't collected a full CPI of data yet
        d_pulse_count++;
    } else {
        // Do processing
        d_processing_thread.join();
        d_processing_thread = gr::thread::thread([this] { process_data(); });
        // Reset the pulse counter
        d_pulse_count = 0;
    }
}

void match_filt_impl::process_data()
{
    // Resize the FFT plans if necessary
    size_t n = d_fast_slow_time.rows();
    size_t nfft = n + d_match_filt.size() - 1;
    fftresize(nfft);

    // Convolve each pulse with the matched filter
    Eigen::ArrayXXcf range_slow_time(nfft, d_num_pulse_cpi);
    for (size_t ipulse = 0; ipulse < d_num_pulse_cpi; ipulse++) {
        range_slow_time.col(ipulse) = Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(
            conv(d_fast_slow_time.col(ipulse).data(), n).data(), nfft);
    }
    range_slow_time *= 1 / (float)nfft;

    // Send the data
    pmt::pmt_t data = pmt::init_c32vector(range_slow_time.size(), range_slow_time.data());
    pmt::pmt_t out = pmt::cons(pmt::make_dict(), data);
    message_port_pub(pmt::mp("out"), out);
}

std::vector<gr_complex> match_filt_impl::conv(const gr_complex* x, size_t nx)
{
    // Zero-pad the input and transform
    memcpy(d_fwd->get_inbuf(), x, nx * sizeof(gr_complex));
    for (size_t i = nx; i < d_fftsize; i++)
        d_fwd->get_inbuf()[i] = 0;
    d_fwd->execute();
    gr_complex* a = d_fwd->get_outbuf();

    // Get the matched filter (already transformed and zero-padded)
    gr_complex* b = d_match_filt_freq.data();

    // Hadamard and IFFT
    volk_32fc_x2_multiply_32fc_a(d_inv->get_inbuf(), a, b, d_fftsize);
    d_inv->execute();

    // Copy the result to
    std::vector<gr_complex> out(d_fftsize);
    // gr_complex* out = new gr_complex[d_fftsize];
    memcpy(out.data(), d_inv->get_outbuf(), d_fftsize * sizeof(gr_complex));

    return out;
}

void match_filt_impl::fftresize(size_t size)
{
    gr::thread::scoped_lock lock(d_setlock);

    if (size != d_fftsize) {
        // Re-initialize the plans
        d_fftsize = size;
        d_fwd = std::make_unique<fft::fft_complex_fwd>(size);
        d_inv = std::make_unique<fft::fft_complex_rev>(size);

        // Pre-compute the frequency-domain matched filter for the current FFT size
        memcpy(d_fwd->get_inbuf(),
               d_match_filt.data(),
               d_match_filt.size() * sizeof(gr_complex));
        for (auto i = d_match_filt.size(); i < d_fwd->inbuf_length(); i++)
            d_fwd->get_inbuf()[i] = 0;
        d_fwd->execute();
        d_match_filt_freq =
            Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(d_fwd->get_outbuf(), size);
    }
}

} /* namespace plasma */
} /* namespace gr */

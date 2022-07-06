/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "match_filt_impl.h"
#include <gnuradio/io_signature.h>
#include <chrono>
#include <future>

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
    d_data = pmt::make_c32vector(1, 0);
    d_meta = pmt::make_dict();
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
    return block::stop();
}

int nextpow2(int x) { return pow(2, ceil(log(x) / log(2))); }

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
    d_match_filt = Eigen::Map<Eigen::ArrayXcf>(data.data(), n).conjugate().reverse();
}

void match_filt_impl::handle_rx_msg(pmt::pmt_t msg)
{
    
    // auto start = std::chrono::high_resolution_clock::now();
    pmt::pmt_t samples;

    if (d_match_filt.size() == 0 or this->nmsgs(pmt::intern("rx")) >= d_msg_queue_depth) {
        return;
    }
    // Get a copy of the input samples
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    process_data(samples);
    d_pulse_count++;
    // Send the full CPI to the output port
    if (d_pulse_count == d_num_pulse_cpi) {
        message_port_pub(pmt::intern("out"), pmt::cons(d_meta, d_data));
        d_pulse_count = 0;
    }

    // auto stop = std::chrono::high_resolution_clock::now();
    // GR_LOG_DEBUG(d_logger, this->nmsgs(pmt::intern("rx")))
    // GR_LOG_DEBUG(d_logger, std::chrono::duration<double>(stop - start).count())
}

void match_filt_impl::process_data(pmt::pmt_t data)
{
    size_t n = pmt::length(data);
    gr_complex* in = pmt::c32vector_writable_elements(data, n);
    Eigen::Map<Eigen::ArrayXcf> fast_slow_time(in, n);
    // fast_slow_time.setZero();

    auto nfft = n + d_match_filt.size() - 1;
    fftresize(nextpow2(nfft));
    // Zero-pad the input and transform
    memcpy(d_fwd->get_inbuf(), fast_slow_time.data(), n * sizeof(gr_complex));
    for (size_t i = n; i < d_fftsize; i++)
        d_fwd->get_inbuf()[i] = 0;
    d_fwd->execute();
    gr_complex* a = d_fwd->get_outbuf();

    // Get the matched filter (already transformed and zero-padded)
    gr_complex* b = d_match_filt_freq.data();

    // Hadamard and IFFT
    volk_32fc_x2_multiply_32fc_a(d_inv->get_inbuf(), a, b, d_fftsize);
    d_inv->execute();

    volk_32fc_s32fc_multiply_32fc_a(
        d_inv->get_outbuf(), d_inv->get_outbuf(), 1 / (float)d_fftsize, d_fftsize);

    // Copy the result to the output buffer
    if (pmt::length(d_data) != nfft * d_num_pulse_cpi) {
        d_data = pmt::make_c32vector(nfft * d_num_pulse_cpi, 0);
    }
    size_t io(0);
    memcpy(pmt::c32vector_writable_elements(d_data, io) + nfft * d_pulse_count,
           d_inv->get_outbuf(),
           nfft * sizeof(gr_complex));
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

void match_filt_impl::set_msg_queue_depth(size_t depth) {
    d_msg_queue_depth = depth;

}
} /* namespace plasma */
} /* namespace gr */

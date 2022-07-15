/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "match_filt_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>
#include <chrono>
#include <future>

namespace gr {
namespace plasma {

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
    d_data = pmt::make_c32vector(1, 0);
    d_meta = pmt::make_dict();
    message_port_register_in(PMT_TX);
    message_port_register_in(PMT_RX);
    message_port_register_out(PMT_OUT);
    set_msg_handler(PMT_TX, [this](pmt::pmt_t msg) { handle_tx_msg(msg); });
    set_msg_handler(PMT_RX, [this](pmt::pmt_t msg) { handle_rx_msg(msg); });
}

/*
 * Our virtual destructor.
 */
match_filt_impl::~match_filt_impl() {}


int nextpow2(int x) { return pow(2, ceil(log(x) / log(2))); }

void match_filt_impl::handle_tx_msg(pmt::pmt_t msg)
{
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        // Get the transmit data
        samples = pmt::cdr(msg);
        d_meta = pmt::dict_update(d_meta, pmt::car(msg));
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    size_t n = pmt::length(samples);
    size_t io(0);
    gr_complex* tx_data = pmt::c32vector_writable_elements(samples, io);
    // Resize the matched filter array if necessary
    if (d_match_filt.elements() != (int)n) {
        d_match_filt = af::constant(0, n, c32);
    }
    // Create the matched filter
    d_match_filt.write(reinterpret_cast<af::cfloat*>(tx_data), n * sizeof(gr_complex));
    d_match_filt = af::conjg(d_match_filt);
    d_match_filt = af::flip(d_match_filt, 0);
}

void match_filt_impl::handle_rx_msg(pmt::pmt_t msg)
{
    pmt::pmt_t samples;
    if (d_match_filt.elements() == 0 or this->nmsgs(PMT_RX) > d_msg_queue_depth) {
        return;
    }
    // Get a copy of the input samples
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
        d_meta = pmt::dict_update(d_meta, pmt::car(msg));
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    // Compute matrix and vector dimensions
    size_t n = pmt::length(samples);
    size_t ncol = d_num_pulse_cpi;
    size_t nrow = n / ncol;
    size_t nconv = nrow + d_match_filt.elements() - 1;
    if (pmt::length(d_data) != n)
        d_data = pmt::make_c32vector(nconv*ncol, 0);
    
    // Get input and output data
    size_t io(0);
    const gr_complex* in = pmt::c32vector_elements(samples, io);
    gr_complex* out = pmt::c32vector_writable_elements(d_data, io);

    // Apply the matched filter to each column
    af::array a = af::constant(0, nrow, ncol, c32);
    a.write(reinterpret_cast<const af::cfloat*>(in), n * sizeof(gr_complex));
    a = af::convolve1(a, d_match_filt, AF_CONV_EXPAND, AF_CONV_AUTO);
    a.host(out);

    message_port_pub(PMT_OUT, pmt::cons(d_meta, d_data));
    d_meta = pmt::make_dict();
}

void match_filt_impl::set_msg_queue_depth(size_t depth) { d_msg_queue_depth = depth; }
} /* namespace plasma */
} /* namespace gr */

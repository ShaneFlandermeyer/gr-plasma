/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "doppler_processing_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

doppler_processing::sptr doppler_processing::make(size_t num_pulse_cpi, size_t nfft)
{
    return gnuradio::make_block_sptr<doppler_processing_impl>(num_pulse_cpi, nfft);
}


/*
 * The private constructor
 */
doppler_processing_impl::doppler_processing_impl(size_t num_pulse_cpi, size_t nfft)
    : gr::block("doppler_processing",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_num_pulse_cpi(num_pulse_cpi),
      d_nfft(nfft)
{
    message_port_register_in(pmt::mp("in"));
    message_port_register_out(pmt::mp("out"));
    set_msg_handler(pmt::mp("in"), [this](pmt::pmt_t msg) { handle_msg(msg); });
}

/*
 * Our virtual destructor.
 */
doppler_processing_impl::~doppler_processing_impl() {}

void doppler_processing_impl::handle_msg(pmt::pmt_t msg)
{
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
    gr_complex* inptr = pmt::c32vector_writable_elements(samples, n);
    d_in_data = Eigen::Map<Eigen::ArrayXXcf>(inptr, n/d_num_pulse_cpi, d_num_pulse_cpi);
    GR_LOG_DEBUG(d_logger, d_in_data.size())
}
} /* namespace plasma */
} /* namespace gr */

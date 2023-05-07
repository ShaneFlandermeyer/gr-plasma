/* -*- c++ -*- */
/*
 * Copyright 2023 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cw_to_pulsed_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

cw_to_pulsed::sptr cw_to_pulsed::make(double prf, double samp_rate)
{
    return gnuradio::make_block_sptr<cw_to_pulsed_impl>(prf, samp_rate);
}


/*
 * The private constructor
 */
cw_to_pulsed_impl::cw_to_pulsed_impl(double prf, double samp_rate)
    : gr::block("cw_to_pulsed",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_prf(prf),
      d_sample_rate(samp_rate),
      d_in_port(PMT_IN),
      d_out_port(PMT_OUT)
{
    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);
    set_msg_handler(d_in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

void cw_to_pulsed_impl::handle_msg(pmt::pmt_t msg)
{
    pmt::pmt_t meta, samples;
    if (pmt::is_pair(msg)) {
        meta = pmt::car(msg);
        samples = pmt::cdr(msg);
    } else {
        meta = pmt::PMT_NIL;
        samples = msg;
    }
    size_t n_nonzero = pmt::length(samples);

    // Update PRI parameters from metadata
    parse_meta(meta);
    meta = pmt::dict_add(meta, d_sample_rate_key, pmt::from_double(d_sample_rate));
    meta = pmt::dict_add(meta, d_prf_key, pmt::from_double(d_prf));
    meta = pmt::dict_add(meta, d_nonzero_key, pmt::from_long(n_nonzero));

    // Copy the data into a zero-padded vector
    size_t n_pri = round(d_sample_rate / d_prf);
    std::vector<gr_complex> data(n_pri, 0);
    std::copy(pmt::c32vector_elements(samples, n_nonzero),
              pmt::c32vector_elements(samples, n_nonzero) + n_nonzero,
              data.begin());
    pmt::pmt_t pdu_data = pmt::init_c32vector(n_pri, data.data());

    // Send the message
    message_port_pub(d_out_port, pmt::cons(meta, pdu_data));
}

void cw_to_pulsed_impl::parse_meta(pmt::pmt_t meta)
{
    if (pmt::is_dict(meta)) {
        pmt::pmt_t prf = pmt::dict_ref(meta, pmt::string_to_symbol("prf"), pmt::PMT_NIL);
        if (prf != pmt::PMT_NIL) {
            d_prf = pmt::to_double(prf);
        }
        pmt::pmt_t samp_rate =
            pmt::dict_ref(meta, pmt::string_to_symbol("samp_rate"), pmt::PMT_NIL);
        if (samp_rate != pmt::PMT_NIL) {
            d_sample_rate = pmt::to_double(samp_rate);
        }
    }
}

void cw_to_pulsed_impl::init_meta_dict(const std::string& sample_rate_key,
                                       const std::string& prf_key)
{
    d_sample_rate_key = pmt::intern(sample_rate_key);
    d_prf_key = pmt::intern(prf_key);
}

} /* namespace plasma */
} /* namespace gr */

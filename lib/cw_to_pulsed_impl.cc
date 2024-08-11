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
      prf(prf),
      sample_rate(samp_rate),
      last_data(pmt::PMT_NIL),
      in_port(PMT_IN),
      out_port(PMT_OUT)

{
    message_port_register_in(in_port);
    message_port_register_out(out_port);
    set_msg_handler(in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

void cw_to_pulsed_impl::handle_msg(pmt::pmt_t msg)
{
    pmt::pmt_t meta, samples;

    if (pmt::is_pair(msg)) {
        meta = pmt::car(msg);
        samples = pmt::cdr(msg);
    } else if (pmt::is_dict(msg)) {
        meta = msg;
        samples = pmt::PMT_NIL;
    } else if (pmt::is_vector(msg)) {
        meta = pmt::PMT_NIL;
        samples = msg;
    } else {
        GR_LOG_ERROR(d_logger, "Unexpected message type")
    }

    // Update with new data
    if (samples != pmt::PMT_NIL) {
        // Data vector updated
        last_data = samples;
    }

    size_t n_nonzero = pmt::length(last_data);

    // Update with new meta
    if (meta != pmt::PMT_NIL) {
        parse_input_meta(meta);
        meta = pmt::dict_add(meta, sample_rate_key, pmt::from_double(sample_rate));
        meta = pmt::dict_add(meta, prf_key, pmt::from_double(prf));
        meta = pmt::dict_add(meta, nonzero_key, pmt::from_long(n_nonzero));
    }

    // Send new message (if data is available)
    if (last_data != pmt::PMT_NIL) {
        size_t n_pri = round(sample_rate / prf);
        std::vector<gr_complex> data(n_pri, 0);
        std::copy(pmt::c32vector_elements(last_data, n_nonzero),
                  pmt::c32vector_elements(last_data, n_nonzero) + n_nonzero,
                  data.begin());
        pmt::pmt_t pdu_data = pmt::init_c32vector(n_pri, data.data());

        message_port_pub(out_port, pmt::cons(meta, pdu_data));
    }
}


void cw_to_pulsed_impl::parse_input_meta(pmt::pmt_t meta)
{
    pmt::pmt_t prf, samp_rate;
    prf = pmt::dict_ref(meta, pmt::string_to_symbol("prf"), pmt::PMT_NIL);
    if (prf != pmt::PMT_NIL) {
        this->prf = pmt::to_double(prf);
    }
    samp_rate = pmt::dict_ref(meta, pmt::string_to_symbol("samp_rate"), pmt::PMT_NIL);
    if (samp_rate != pmt::PMT_NIL) {
        this->sample_rate = pmt::to_double(samp_rate);
    }
}

void cw_to_pulsed_impl::init_meta_dict(const std::string& sample_rate_key,
                                       const std::string& prf_key)
{
    this->sample_rate_key = pmt::intern(sample_rate_key);
    this->prf_key = pmt::intern(prf_key);
}

} /* namespace plasma */
} /* namespace gr */

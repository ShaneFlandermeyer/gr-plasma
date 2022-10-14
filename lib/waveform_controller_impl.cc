/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "waveform_controller_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/pdu.h>

namespace gr {
namespace plasma {

using output_type = gr_complex;
waveform_controller::sptr waveform_controller::make(double prf, double samp_rate)
{
    return gnuradio::make_block_sptr<waveform_controller_impl>(prf, samp_rate);
}


/*
 * The private constructor
 */
waveform_controller_impl::waveform_controller_impl(double prf, double samp_rate)
    : gr::block("waveform_controller",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0))
{

    d_prf = prf;
    d_samp_rate = samp_rate;
    d_num_samp_pri = round(samp_rate / prf);
    d_updated = false;

    d_in_port = PMT_IN;
    d_out_port = PMT_OUT;
    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);
    set_msg_handler(d_in_port, [this](const pmt::pmt_t& msg) { handle_message(msg); });
}

/*
 * Our virtual destructor.
 */
waveform_controller_impl::~waveform_controller_impl() {}

void waveform_controller_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t meta = pmt::car(msg);
        pmt::pmt_t data = pmt::cdr(msg);
        size_t n = pmt::length(data);
        d_num_samp_waveform = pmt::length(data);

        // Add metadata to the existing PDU
        meta = pmt::dict_update(meta, d_meta);

        message_port_pub(
            d_out_port,
            pmt::cons(meta, pmt::init_c32vector(n, pmt::c32vector_elements(data, n))));
    } else {
        GR_LOG_ERROR(d_logger, "Waveform controller input must be a PDU")
    }
}

void waveform_controller_impl::init_meta_dict(std::string prf_key,
                                                 std::string samp_rate_key)
{
    d_prf_key = pmt::intern(prf_key);
    d_sample_rate_key = pmt::intern(samp_rate_key);

    // Add metadata to the dictionary
    d_meta = pmt::make_dict();
    d_meta = pmt::dict_add(d_meta, d_prf_key, pmt::from_double(d_prf));
    d_meta = pmt::dict_add(d_meta, d_sample_rate_key, pmt::from_double(d_samp_rate));
}

} /* namespace plasma */
} /* namespace gr */

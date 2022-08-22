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


    d_updated = false;
    d_prf = prf;
    d_samp_rate = samp_rate;
    d_num_samp_pri = round(samp_rate / prf);

    d_annotations = pmt::make_dict();
    d_annotations = pmt::dict_add(d_annotations, PMT_PRF, pmt::from_double(prf));

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

// bool waveform_controller_impl::start()
// {
//     d_finished = false;
//     return block::start();
// }

// bool waveform_controller_impl::stop()
// {
//     d_finished = true;
//     return block::stop();
// }

void waveform_controller_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t meta = pmt::car(msg);
        pmt::pmt_t data = pmt::cdr(msg);
        size_t n = pmt::length(data);
        d_num_samp_waveform = pmt::length(data);

        // Update the radar annotations in the metadata
        if (pmt::dict_has_key(meta, PMT_ANNOTATIONS)) {
            pmt::pmt_t annotations = pmt::dict_ref(meta, PMT_ANNOTATIONS, pmt::PMT_NIL);
            annotations = pmt::dict_update(annotations, d_annotations);
            meta = pmt::dict_add(meta, PMT_ANNOTATIONS, annotations);
        } else {
            meta = pmt::dict_add(meta, PMT_ANNOTATIONS, d_annotations);
        }

        message_port_pub(
            d_out_port,
            pmt::cons(meta, pmt::init_c32vector(n, pmt::c32vector_elements(data, n))));


    } else {
        GR_LOG_ERROR(d_logger, "Waveform controller input must be a PDU")
    }
}

} /* namespace plasma */
} /* namespace gr */

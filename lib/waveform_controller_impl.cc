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
    : gr::sync_block("waveform_controller",
                     gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(
                         1 /* min outputs */, 1 /*max outputs */, sizeof(output_type)))
{
    message_port_register_in(pmt::mp("in"));
    message_port_register_out(pmt::mp("out"));
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    d_sample_index = 0;
    d_updated = false;
    d_prf = prf;
    d_samp_rate = samp_rate;
    d_num_samp_pri = round(samp_rate / prf);
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
        d_num_samp_waveform = pmt::length(data);
        // Convert from pmt to a pointer. The second argument of
        // uniform_vector_elements expects an lvalue, so the dummy variable must
        // be created below
        size_t zero(0);
        output_type* ptr = (output_type*)pmt::uniform_vector_elements(data, zero);
        // Create a std vector and zero-pad to the length of the PRI
        d_data = std::vector<output_type>(ptr, ptr + d_num_samp_waveform);
        d_data.insert(d_data.end(), d_num_samp_pri - d_num_samp_waveform, 0);

        // Send the new waveform through the message port (with additional
        // metadata)
        meta = pmt::dict_add(meta, pmt::intern("prf"), pmt::from_double(d_prf));
        // pmt::pmt_t data = ;
        message_port_pub(pmt::mp("out"), pmt::cons(meta, pmt::init_c32vector(d_data.size(), d_data.data())));
        
    } else if (pmt::is_pair(msg)) {
        pmt::pmt_t key = pmt::car(msg);
        pmt::pmt_t val = pmt::cdr(msg);
        if (pmt::eq(key, pmt::mp("prf"))) {
            double new_prf = pmt::to_double(val);
            if (round(d_samp_rate / new_prf) > d_num_samp_waveform) {
                d_prf = new_prf;
                d_num_samp_pri = round(d_samp_rate / d_prf);
                d_updated = true;
            } else {
                GR_LOG_ERROR(d_logger, "PRI is less than the waveform duration");
            }
        }
    }
}

int waveform_controller_impl::work(int noutput_items,
                                   gr_vector_const_void_star& input_items,
                                   gr_vector_void_star& output_items)
{
    auto out = static_cast<output_type*>(output_items[0]);
    for (int i = 0; i < noutput_items; i++) {
        if (d_sample_index == 0) {
            if (d_updated) {
                // Update the waveform's zero-padding
                d_data.resize(d_num_samp_waveform);
                d_data.insert(d_data.end(), d_num_samp_pri - d_num_samp_waveform, 0);
                d_updated = false;
            }
            // Propagate metadata through tags
            add_item_tag(
                0, nitems_written(0) + i, pmt::intern("prf"), pmt::from_double(d_prf));
            add_item_tag(0,
                         nitems_written(0) + i,
                         pmt::intern("samp_rate"),
                         pmt::from_double(d_samp_rate));
        }
        out[i] = d_data[d_sample_index];
        d_sample_index = (d_sample_index + 1) % d_data.size();
    }

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace plasma */
} /* namespace gr */

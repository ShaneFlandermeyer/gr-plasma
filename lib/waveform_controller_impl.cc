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
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    d_sample_index = 0;
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
    // Handle new waveform PDU
    if (pmt::is_pair(msg)) {
        pmt::pmt_t meta = pmt::car(msg);
        pmt::pmt_t data = pmt::cdr(msg);
        d_num_samp_waveform = pmt::length(data);

        size_t io(0);
        output_type* ptr = (output_type*)pmt::uniform_vector_elements(data, io);
        d_data = std::vector<output_type>(ptr, ptr + d_num_samp_waveform);
        d_data.insert(d_data.end(), d_num_samp_pri - d_num_samp_waveform, 0);
    }
}

int waveform_controller_impl::work(int noutput_items,
                                   gr_vector_const_void_star& input_items,
                                   gr_vector_void_star& output_items)
{
    auto out = static_cast<output_type*>(output_items[0]);
    for (int i = 0; i < noutput_items; i++) {
        if (d_sample_index == 0) {
            // TODO: Propagate metadata through tags
        }
        out[i] = d_data[d_sample_index];
        d_sample_index = (d_sample_index + 1) % d_data.size();
        
    }

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace plasma */
} /* namespace gr */

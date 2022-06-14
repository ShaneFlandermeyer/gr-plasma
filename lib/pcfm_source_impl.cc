/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pcfm_source_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

#pragma message("set the following appropriately and remove this warning")
using input_type = float;
#pragma message("set the following appropriately and remove this warning")
using output_type = float;
pcfm_source::sptr pcfm_source::make(const std::vector<double>& code,
                                    const std::vector<double>& filter,
                                    double samp_rate)
{
    return gnuradio::make_block_sptr<pcfm_source_impl>(code, filter, samp_rate);
}


/*
 * The private constructor
 */
pcfm_source_impl::pcfm_source_impl(const std::vector<double>& code,
                                   const std::vector<double>& filter,
                                   double samp_rate)
    : gr::block(
          "pcfm_source", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0))
{
}

bool pcfm_source_impl::start()
{
#pragma message("TODO: Send data and metadata as PDU");
    return block::start();
    // d_finished = false;
    // // Send a PDU containing the waveform and its metadata
    // pmt::pmt_t meta = pmt::make_dict();
    // meta = pmt::dict_add(
    //     meta, pmt::intern("bandwidth"), pmt::from_double(d_waveform.bandwidth()));
    // meta = pmt::dict_add(
    //     meta, pmt::intern("pulse_width"), pmt::from_double(d_waveform.pulse_width()));
    // meta = pmt::dict_add(
    //     meta, pmt::intern("samp_rate"), pmt::from_double(d_waveform.samp_rate()));
    // pmt::pmt_t data = pmt::init_c32vector(d_data.size(), d_data.data());
    // message_port_pub(d_port, pmt::cons(meta, data));


    // return block::start();
}

// /*
//  * Our virtual destructor.
//  */
// pcfm_source_impl::~pcfm_source_impl() {}

// void pcfm_source_impl::forecast(int noutput_items, gr_vector_int&
// ninput_items_required)
// {
// #pragma message( \
//     "implement a forecast that fills in how many items on each input you need to
//     produce noutput_items and remove this warning")
//     /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
// }

// int pcfm_source_impl::general_work(int noutput_items,
//                                    gr_vector_int& ninput_items,
//                                    gr_vector_const_void_star& input_items,
//                                    gr_vector_void_star& output_items)
// {
//     auto in = static_cast<const input_type*>(input_items[0]);
//     auto out = static_cast<output_type*>(output_items[0]);

// #pragma message("Implement the signal processing in your block and remove this
// warning")
//     // Do <+signal processing+>
//     // Tell runtime system how many input items we consumed on
//     // each input stream.
//     consume_each(noutput_items);

//     // Tell runtime system how many output items we produced.
//     return noutput_items;
// }

} /* namespace plasma */
} /* namespace gr */

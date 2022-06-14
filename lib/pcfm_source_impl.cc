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
pcfm_source::sptr pcfm_source::make(std::vector<double>& code,
                                    std::vector<double>& filter)
{
    return gnuradio::make_block_sptr<pcfm_source_impl>(code, filter);
}


/*
 * The private constructor
 */
pcfm_source_impl::pcfm_source_impl(std::vector<double>& code, std::vector<double>& filter)
    : gr::block("pcfm_source",
                gr::io_signature::make(
                    1 /* min inputs */, 1 /* max inputs */, sizeof(input_type)),
                gr::io_signature::make(
                    1 /* min outputs */, 1 /*max outputs */, sizeof(output_type)))
{
    d_out_port = pmt::mp("out");
    message_port_register_out(d_out_port);
}

/*
 * Our virtual destructor.
 */
pcfm_source_impl::~pcfm_source_impl() {}

void pcfm_source_impl::forecast(int noutput_items, gr_vector_int& ninput_items_required)
{
#pragma message( \
    "implement a forecast that fills in how many items on each input you need to produce noutput_items and remove this warning")
    /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
}

int pcfm_source_impl::general_work(int noutput_items,
                                   gr_vector_int& ninput_items,
                                   gr_vector_const_void_star& input_items,
                                   gr_vector_void_star& output_items)
{
    auto in = static_cast<const input_type*>(input_items[0]);
    auto out = static_cast<output_type*>(output_items[0]);

#pragma message("Implement the signal processing in your block and remove this warning")
    // Do <+signal processing+>
    // Tell runtime system how many input items we consumed on
    // each input stream.
    consume_each(noutput_items);

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace plasma */
} /* namespace gr */

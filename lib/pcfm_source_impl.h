/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H
#define INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H

#include <gnuradio/plasma/pcfm_source.h>

namespace gr {
namespace plasma {

class pcfm_source_impl : public pcfm_source
{
private:
    // Nothing to declare in this block.
    pmt::pmt_t d_out_port;

public:
    pcfm_source_impl(std::vector<double>& code, std::vector<double>& filter);
    ~pcfm_source_impl();

    // Where all the action really happens
    void forecast(int noutput_items, gr_vector_int& ninput_items_required);

    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H
#define INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H

#include <gnuradio/plasma/pcfm_source.h>
#include <plasma_dsp/pcfm.h>

namespace gr {
namespace plasma {

class pcfm_source_impl : public pcfm_source
{
private:
    

public:
    pcfm_source_impl(const std::vector<double>& code,
                     const std::vector<double>& filter,
                     double samp_rate);
    ~pcfm_source_impl();

    bool start() override;

    // Where all the action really happens
    // void forecast(int noutput_items, gr_vector_int& ninput_items_required);

    // int general_work(int noutput_items,
    //                  gr_vector_int& ninput_items,
    //                  gr_vector_const_void_star& input_items,
    //                  gr_vector_void_star& output_items);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H */

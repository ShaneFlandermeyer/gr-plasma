/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H
#define INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H

#include <gnuradio/plasma/waveform_controller.h>

namespace gr {
namespace plasma {

class waveform_controller_impl : public waveform_controller
{
private:
    // Nothing to declare in this block.
    double d_prf;
    double d_samp_rate;
    size_t d_num_samp_pri;
    size_t d_num_samp_waveform;
    size_t d_sample_index;
    std::vector<gr_complex> d_data;

    void handle_message(const pmt::pmt_t& msg);

public:
    waveform_controller_impl(double prf, double samp_rate);
    ~waveform_controller_impl();

    // Where all the action really happens
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H
#define INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H

#include <gnuradio/plasma/pcfm_source.h>
#include <gnuradio/plasma/pmt_dict_keys.h>
#include <plasma_dsp/pcfm.h>
#include <plasma_dsp/phase_code.h>

namespace gr {
namespace plasma {

class pcfm_source_impl : public pcfm_source
{
private:
    pmt::pmt_t d_out_port;
    pmt::pmt_t d_meta;
    ::plasma::PCFMWaveform d_waveform;
    Eigen::ArrayXcf d_data;
    Eigen::ArrayXd d_code;
    Eigen::ArrayXd d_filter;
    double d_samp_rate;
    std::atomic<bool> d_finished;

public:
    pcfm_source_impl(std::vector<double>& code,
                     std::vector<double>& filter,
                     double samp_rate);
    ~pcfm_source_impl();

    bool start() override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H */

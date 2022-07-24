/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H
#define INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H

#include <gnuradio/plasma/pcfm_source.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <plasma_dsp/pcfm.h>
#include <plasma_dsp/phase_code.h>

namespace gr {
namespace plasma {

class pcfm_source_impl : public pcfm_source
{
private:
    
    ::plasma::PCFMWaveform d_waveform;
    std::unique_ptr<gr_complex> d_data;
    size_t d_num_samp;
    af::array d_code;
    af::array d_filter;
    double d_samp_rate;
    std::atomic<bool> d_finished;

    pmt::pmt_t d_global;
    pmt::pmt_t d_annotations;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_out_port;

public:
    pcfm_source_impl(PhaseCode::Code code,
                     int n,
                     int over,
                     double samp_rate);
    ~pcfm_source_impl();

    bool start() override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_LFM_SOURCE_IMPL_H
#define INCLUDED_PLASMA_LFM_SOURCE_IMPL_H

#include <gnuradio/plasma/lfm_source.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <arrayfire.h>
#include <plasma_dsp/lfm.h>

namespace gr {
namespace plasma {


class lfm_source_impl : public lfm_source
{
private:
    // Message ports
    const pmt::pmt_t msg_port;
    const pmt::pmt_t out_port;
    pmt::pmt_t d_msg;

    // Waveform parameters
    double bandwidth;
    double start_freq;
    double pulse_width;
    double samp_rate;
    double prf;

    // Waveform object and IQ data
    pmt::pmt_t pmt_samples;

    // Metadata fields
    pmt::pmt_t meta;
    pmt::pmt_t label_key;
    pmt::pmt_t sample_rate_key;
    pmt::pmt_t bandwidth_key;
    pmt::pmt_t start_freq_key;
    pmt::pmt_t duration_key;
    pmt::pmt_t prf_key;
    

    void handle_msg(pmt::pmt_t msg);


public:
    lfm_source_impl(double bandwidth,
                    double start_freq,
                    double pulse_width,
                    double samp_rate,
                    double prf);
    ~lfm_source_impl();

    bool start() override;

    void init_meta_dict(const std::string& bandwidth_key,
                        const std::string& start_freq_key,
                        const std::string& duration_key,
                        const std::string& sample_rate_key,
                        const std::string& label_key,
                        const std::string& prf_key);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_LFM_SOURCE_IMPL_H */

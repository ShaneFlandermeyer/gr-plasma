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
#include <plasma_dsp/linear_fm_waveform.h>

namespace gr {
namespace plasma {


class lfm_source_impl : public lfm_source
{
private:
    // Message ports
    const pmt::pmt_t d_msg_port;
    const pmt::pmt_t d_out_port;
    pmt::pmt_t d_msg;

    // Waveform parameters
    double d_bandwidth;
    double d_start_freq;
    double d_pulse_width;
    double d_samp_rate;

    // Waveform object and IQ data
    ::plasma::LinearFMWaveform d_waveform;
    std::unique_ptr<std::complex<float>> d_data;
    size_t d_num_samp;
    uint64_t d_start_time;
    uint64_t d_send_time;

    // Metadata fields
    pmt::pmt_t d_label_key;
    pmt::pmt_t d_sample_rate_key;
    pmt::pmt_t d_bandwidth_key;
    pmt::pmt_t d_start_freq_key;
    pmt::pmt_t d_duration_key;
    // Metadata dict(s)
    pmt::pmt_t d_global;
    pmt::pmt_t d_annotations;
    pmt::pmt_t d_meta;

    void handle_msg(pmt::pmt_t msg);


public:
    lfm_source_impl(double bandwidth,
                    double start_freq,
                    double pulse_width,
                    double samp_rate);
    ~lfm_source_impl();

    bool start() override;

    void init_meta_dict(const std::string& bandwidth_key,
                        const std::string& start_freq_key,
                        const std::string& duration_key,
                        const std::string& sample_rate_key,
                        const std::string& label_key);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_LFM_SOURCE_IMPL_H */

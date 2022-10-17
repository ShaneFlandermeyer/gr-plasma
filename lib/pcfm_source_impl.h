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
    size_t d_n_samp;
    size_t d_n_chips;
    af::array d_code;
    af::array d_filter;
    double d_samp_rate;
    std::atomic<bool> d_finished;


    pmt::pmt_t d_meta;
    pmt::pmt_t d_out_port;
    std::string d_code_class;

    // Metadata keys
    pmt::pmt_t d_sample_rate_key;
    pmt::pmt_t d_label_key;
    pmt::pmt_t d_duration_key;
    pmt::pmt_t d_phase_code_class_key;
    pmt::pmt_t d_n_phase_code_chips_key;

public:
    pcfm_source_impl(PhaseCode::Code code, int n, int over, double samp_rate);
    ~pcfm_source_impl();

    bool start() override;

    void set_metadata_keys(const std::string& label_key,
                      const std::string& phase_code_class_key,
                      const std::string& n_phase_code_chips_key,
                      const std::string& duration_key,
                      const std::string& sample_rate_key);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PCFM_SOURCE_IMPL_H */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_LFM_SOURCE_IMPL_H
#define INCLUDED_PLASMA_LFM_SOURCE_IMPL_H

#include <gnuradio/plasma/lfm_source.h>
#include <plasma-dsp/linear_fm_waveform.h>

namespace gr {
namespace plasma {

class lfm_source_impl : public lfm_source
{
private:
    const pmt::pmt_t d_port;
    pmt::pmt_t d_msg;
    gr::thread::thread d_thread;
    ::plasma::LinearFMWaveform d_waveform;
    Eigen::ArrayXcf d_data;
    // boost::posix_time::ptime d_epoch;
    uint64_t d_start_time;
    uint64_t d_send_time;
    double d_prf;
    // size_t d_sample_index;
    std::atomic<bool> d_finished;
    // std::atomic<bool> d_armed;

    void run();

public:
    // TODO: Remove prf parameter
    lfm_source_impl(double bandwidth, double pulse_width, double prf, double samp_rate);
    ~lfm_source_impl();

    bool start() override;
    bool stop() override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_LFM_SOURCE_IMPL_H */

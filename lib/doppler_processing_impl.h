/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H
#define INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H

#include <gnuradio/plasma/doppler_processing.h>
#include <Eigen/Dense>
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>
#include <plasma_dsp/fftshift.h>

namespace gr {
namespace plasma {

class doppler_processing_impl : public doppler_processing
{
private:
    size_t d_num_pulse_cpi;
    size_t d_fftsize;
    size_t d_queue_depth;
    std::unique_ptr<fft::fft_complex_fwd> d_fwd;
    fft::fft_shift<gr_complex> d_shift;

    std::atomic<bool> d_finished;
    gr::thread::thread d_processing_thread;

    void handle_msg(pmt::pmt_t msg);
    void process_data(const Eigen::ArrayXXcf&);
    void fftresize(size_t);

    pmt::pmt_t d_out_port;
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_data;

public:
    doppler_processing_impl(size_t num_pulse_cpi, size_t nfft);
    ~doppler_processing_impl();

    bool start() override;
    bool stop() override;

    void set_msg_queue_depth(size_t) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H */

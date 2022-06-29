/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H
#define INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H

#include "range_doppler_window.h"
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>
#include <gnuradio/filter/fft_filter.h>
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/plasma/range_doppler_sink.h>
#include <gnuradio/thread/thread.h>
#include <plasma_dsp/constants.h>
#include <volk/volk_alloc.hh>


namespace gr {
namespace plasma {

class range_doppler_sink_impl : public range_doppler_sink
{
private:
    double d_samp_rate;
    size_t d_num_pulse_cpi;
    double d_center_freq;
    fft::fft_shift<gr_complex> d_shift;
    int d_argc;
    char* d_argv;
    RangeDopplerWindow* d_main_gui;
    std::unique_ptr<fft::fft_complex_fwd> d_conv_fwd;
    std::unique_ptr<fft::fft_complex_fwd> d_doppler_fft;
    std::unique_ptr<fft::fft_complex_rev> d_conv_inv;
    Eigen::ArrayXcf d_matched_filter;
    Eigen::ArrayXcf d_matched_filter_freq;
    Eigen::ArrayXXcf d_fast_time_slow_time;
    Eigen::ArrayXXcf d_range_slow_time;
    Eigen::ArrayXXcf d_range_doppler;
    std::atomic<size_t> d_count;
    gr::thread::thread d_processing_thread;
    void fftresize(size_t size);
    size_t d_fftsize;

public:
    range_doppler_sink_impl(double samp_rate, size_t num_pulse_cpi, double center_freq, QWidget* parent);
    ~range_doppler_sink_impl();

    void exec_();
    QApplication* d_qapp;
    QWidget* qwidget();
#ifdef ENABLE_PYTHON
    PyObject* pyqwidget();
#else
    void* pyqwidget();
#endif
    void handle_tx_msg(pmt::pmt_t msg);
    void handle_rx_msg(pmt::pmt_t msg);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H */

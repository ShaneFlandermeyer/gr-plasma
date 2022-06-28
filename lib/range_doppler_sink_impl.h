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
#include <gnuradio/filter/fft_filter_ccc.h>
#include <gnuradio/filter/fft_filter.h>
#include <gnuradio/plasma/range_doppler_sink.h>
#include <volk/volk_alloc.hh>


namespace gr {
namespace plasma {

class range_doppler_sink_impl : public range_doppler_sink
{
private:
    int d_argc;
    char* d_argv;
    RangeDopplerWindow* d_main_gui;
    // filter::kernel::fft_filter_ccc d_filter;
    // kernel::fft_filter_ccc d_filter;
    std::unique_ptr<fft::fft_complex_fwd> d_fwd;
    std::unique_ptr<fft::fft_complex_rev> d_inv;
    volk::vector<gr_complex> d_xformed_taps;

    // std::unique_ptr<fft::fft_complex_fwd> d_fft;
    // std::unique_ptr<fft::fft_complex_rev> d_ifft;
    // fft::fft_shift<float> d_fft_shift;

public:
    range_doppler_sink_impl(QWidget* parent);
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
    // void fft(float* data_out, const gr_complex* data_in, int size);

    // volk::vector<gr_complex> d_residbufs;
    // volk::vector<double> d_magbufs;
    Eigen::ArrayXd d_magbuf;
    Eigen::ArrayXcf d_test;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H */

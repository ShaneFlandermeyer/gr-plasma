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
    // Block parameters
    double d_samp_rate;
    size_t d_num_pulse_cpi;
    double d_center_freq;
    double d_dynamic_range_db;
    // GUI parameters
    int d_argc;
    char* d_argv;
    RangeDopplerWindow* d_main_gui;

    std::atomic<bool> d_finished;
    pmt::pmt_t d_in_port;
    size_t d_msg_queue_depth;

    pmt::pmt_t d_meta;

public:
    range_doppler_sink_impl(double samp_rate,
                            size_t num_pulse_cpi,
                            double center_freq,
                            QWidget* parent);
    ~range_doppler_sink_impl();

    bool start() override;
    bool stop() override;

    void exec_();
    QApplication* d_qapp;
    QWidget* qwidget();
#ifdef ENABLE_PYTHON
    PyObject* pyqwidget();
#else
    void* pyqwidget();
#endif
    void handle_rx_msg(pmt::pmt_t msg);

    void set_dynamic_range(const double) override;
    void set_msg_queue_depth(size_t) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H */
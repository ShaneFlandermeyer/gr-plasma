/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H
#define INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H

#include "range_doppler_window.h"
#include <gnuradio/plasma/range_doppler_sink.h>
#include <arrayfire.h>


namespace gr {
namespace plasma {

class range_doppler_sink_impl : public range_doppler_sink
{
private:
    // Block parameters
    double d_samp_rate;
    size_t d_ncol;
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
    // Metadata keys
    pmt::pmt_t d_samp_rate_key;
    pmt::pmt_t d_center_freq_key;
    pmt::pmt_t d_n_matrix_col_key;
    pmt::pmt_t d_dynamic_range_key;


public:
    range_doppler_sink_impl(double samp_rate,
                            size_t ncol,
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
    void set_metadata_keys(std::string samp_rate_key,
                           std::string n_matrix_col_key,
                           std::string center_freq_key,
                           std::string dynamic_range_key,
                           std::string prf_key,
                           std::string pulsewidth_key,
                           std::string detection_indices_key) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H */
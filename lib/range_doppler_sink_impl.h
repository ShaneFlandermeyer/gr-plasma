/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H
#define INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H

#include <gnuradio/plasma/range_doppler_sink.h>
#include "range_doppler_window.h"
#include <volk/volk_alloc.hh>


namespace gr {
namespace plasma {

class range_doppler_sink_impl : public range_doppler_sink
{
private:
    int d_argc;
    char* d_argv;
    // TODO: Replace this with a custom widget
    RangeDopplerWindow* d_main_gui;
    // double xData[plotDataSize];
	// double yData[plotDataSize];

	// long count = 0;

public:
    range_doppler_sink_impl(QWidget* parent);
    ~range_doppler_sink_impl();

    void exec_();
    QApplication *d_qapp;
    QWidget *qwidget();
    #ifdef ENABLE_PYTHON
        PyObject* pyqwidget();
    #else
        void* pyqwidget();
    #endif
    void handle_tx_msg(pmt::pmt_t msg);
    void handle_rx_msg(pmt::pmt_t msg);

    // volk::vector<gr_complex> d_residbufs;
    // volk::vector<double> d_magbufs;
    Eigen::ArrayXd d_magbufs;

    // Where all the action really happens
    // void forecast(int noutput_items, gr_vector_int& ninput_items_required);

    // int general_work(int noutput_items,
    //                  gr_vector_int& ninput_items,
    //                  gr_vector_const_void_star& input_items,
    //                  gr_vector_void_star& output_items);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_DOPPLER_SINK_IMPL_H */

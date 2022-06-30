/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_RANGE_DOPPLER_SINK_H
#define INCLUDED_PLASMA_RANGE_DOPPLER_SINK_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>
#ifdef ENABLE_PYTHON
#pragma push_macro("slots")
#undef slots
#include "Python.h"
#pragma pop_macro("slots")
#endif
// #include <QApplication>
#include <QApplication>
#include <QWidget>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API range_doppler_sink : virtual public gr::block
{
public:
    typedef std::shared_ptr<range_doppler_sink> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::range_doppler_sink.
     *
     * To avoid accidental use of raw pointers, plasma::range_doppler_sink's
     * constructor is in a private implementation
     * class. plasma::range_doppler_sink::make is the public interface for
     * creating new instances.
     */
    static sptr make(double samp_rate,
                     size_t num_pulse_cpi,
                     double center_freq,
                     QWidget* parent = nullptr);
    virtual void exec_() = 0;
    virtual QWidget* qwidget() = 0;
#ifdef ENABLE_PYTHON
    virtual PyObject* pyqwidget() = 0;
#else
    virtual void* pyqwidget() = 0;
#endif

    virtual void set_dynamic_range(const double) = 0;
    virtual void set_num_fft_thread(const int) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_DOPPLER_SINK_H */

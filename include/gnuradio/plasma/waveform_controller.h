/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_WAVEFORM_CONTROLLER_H
#define INCLUDED_PLASMA_WAVEFORM_CONTROLLER_H

#include <gnuradio/plasma/api.h>
#include <gnuradio/block.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API waveform_controller : virtual public gr::block
{
public:
    typedef std::shared_ptr<waveform_controller> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::waveform_controller.
     *
     * To avoid accidental use of raw pointers, plasma::waveform_controller's
     * constructor is in a private implementation
     * class. plasma::waveform_controller::make is the public interface for
     * creating new instances.
     */
    static sptr make(double prf, double samp_rate);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_WAVEFORM_CONTROLLER_H */

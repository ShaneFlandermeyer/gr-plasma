/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_WAVEFORM_CONTROLLER_H
#define INCLUDED_PLASMA_WAVEFORM_CONTROLLER_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief A block that generates a continuous stream of data for the USRP radar
 * block by zero-padding the input waveform to match the number of samples in a
 * pulse repetition interval (PRI).
 * 
 * \details In the future, this will serve as a cognitive radar controller that
 * does more than simple pulse repetition frequency manipulation. 
 * 
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

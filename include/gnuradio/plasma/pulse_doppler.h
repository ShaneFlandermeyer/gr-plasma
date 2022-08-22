/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PULSE_DOPPLER_H
#define INCLUDED_PLASMA_PULSE_DOPPLER_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>
#include <arrayfire.h>
#include <gnuradio/plasma/device.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API pulse_doppler : virtual public gr::block
{
public:
    typedef std::shared_ptr<pulse_doppler> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::pulse_doppler.
     *
     * To avoid accidental use of raw pointers, plasma::pulse_doppler's
     * constructor is in a private implementation
     * class. plasma::pulse_doppler::make is the public interface for
     * creating new instances.
     */
    static sptr make(int num_pulse_cpi, int doppler_fft_size);

    virtual void set_msg_queue_depth(size_t depth) = 0;
    virtual void set_backend(Device::Backend) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PULSE_DOPPLER_H */
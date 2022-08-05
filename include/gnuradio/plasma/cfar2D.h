/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_CFAR2D_H
#define INCLUDED_PLASMA_CFAR2D_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>
#include <gnuradio/plasma/device.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API cfar2D : virtual public gr::block
{
public:
    typedef std::shared_ptr<cfar2D> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::cfar2D.
     *
     * To avoid accidental use of raw pointers, plasma::cfar2D's
     * constructor is in a private implementation
     * class. plasma::cfar2D::make is the public interface for
     * creating new instances.
     */
    static sptr make(std::vector<int> &guard_win_size, std::vector<int> &train_win_size, double pfa, size_t num_pulse_cpi);

    virtual void set_msg_queue_depth(size_t) = 0;

    virtual void set_backend(Device::Backend) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_CFAR2D_H */

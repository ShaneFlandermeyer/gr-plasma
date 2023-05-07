/* -*- c++ -*- */
/*
 * Copyright 2023 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_SIMULATE_RX_H
#define INCLUDED_PLASMA_SIMULATE_RX_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API simulate_rx : virtual public gr::block
{
public:
    typedef std::shared_ptr<simulate_rx> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::simulate_rx.
     *
     * To avoid accidental use of raw pointers, plasma::simulate_rx's
     * constructor is in a private implementation
     * class. plasma::simulate_rx::make is the public interface for
     * creating new instances.
     */
    static sptr make(double delay, double scale);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_SIMULATE_RX_H */

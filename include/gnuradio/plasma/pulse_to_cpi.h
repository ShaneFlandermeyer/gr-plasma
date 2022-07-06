/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PULSE_TO_CPI_H
#define INCLUDED_PLASMA_PULSE_TO_CPI_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API pulse_to_cpi : virtual public gr::block
{
public:
    typedef std::shared_ptr<pulse_to_cpi> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::pulse_to_cpi.
     *
     * To avoid accidental use of raw pointers, plasma::pulse_to_cpi's
     * constructor is in a private implementation
     * class. plasma::pulse_to_cpi::make is the public interface for
     * creating new instances.
     */
    static sptr make(size_t num_pulse_cpi);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PULSE_TO_CPI_H */

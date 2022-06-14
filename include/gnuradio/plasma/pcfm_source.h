/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PCFM_SOURCE_H
#define INCLUDED_PLASMA_PCFM_SOURCE_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API pcfm_source : virtual public gr::block
{
public:
    typedef std::shared_ptr<pcfm_source> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::pcfm_source.
     *
     * To avoid accidental use of raw pointers, plasma::pcfm_source's
     * constructor is in a private implementation
     * class. plasma::pcfm_source::make is the public interface for
     * creating new instances.
     */
    static sptr make(std::vector<double>& code, std::vector<double>& filter, double samp_rate);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PCFM_SOURCE_H */

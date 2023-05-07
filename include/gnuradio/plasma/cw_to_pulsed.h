/* -*- c++ -*- */
/*
 * Copyright 2023 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_CW_TO_PULSED_H
#define INCLUDED_PLASMA_CW_TO_PULSED_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API cw_to_pulsed : virtual public gr::block
{
public:
    typedef std::shared_ptr<cw_to_pulsed> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::cw_to_pulsed.
     *
     * To avoid accidental use of raw pointers, plasma::cw_to_pulsed's
     * constructor is in a private implementation
     * class. plasma::cw_to_pulsed::make is the public interface for
     * creating new instances.
     */
    static sptr make(double prf, double samp_rate);

    virtual void init_meta_dict(const std::string& sample_rate_key,
                                const std::string& prf_key) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_CW_TO_PULSED_H */

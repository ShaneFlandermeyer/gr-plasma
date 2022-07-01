/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_DOPPLER_PROCESSING_H
#define INCLUDED_PLASMA_DOPPLER_PROCESSING_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API doppler_processing : virtual public gr::block
{
public:
    typedef std::shared_ptr<doppler_processing> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::doppler_processing.
     *
     * To avoid accidental use of raw pointers, plasma::doppler_processing's
     * constructor is in a private implementation
     * class. plasma::doppler_processing::make is the public interface for
     * creating new instances.
     */
    static sptr make(size_t num_pulse_cpi, size_t nfft);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_DOPPLER_PROCESSING_H */

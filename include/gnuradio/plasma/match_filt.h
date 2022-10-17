/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_MATCH_FILT_H
#define INCLUDED_PLASMA_MATCH_FILT_H

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
class PLASMA_API match_filt : virtual public gr::block
{
public:
    typedef std::shared_ptr<match_filt> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::match_filt.
     *
     * To avoid accidental use of raw pointers, plasma::match_filt's
     * constructor is in a private implementation
     * class. plasma::match_filt::make is the public interface for
     * creating new instances.
     */
    static sptr make(size_t num_pulse_cpi);

    virtual void set_msg_queue_depth(size_t depth) = 0;
    virtual void set_backend(Device::Backend) = 0;
    virtual void set_metadata_keys(const std::string& n_pulse_cpi_key) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_MATCH_FILT_H */

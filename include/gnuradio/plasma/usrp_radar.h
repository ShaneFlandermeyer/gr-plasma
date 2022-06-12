/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_USRP_RADAR_H
#define INCLUDED_PLASMA_USRP_RADAR_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief A block that simultaneously transmits and receives data from a USRP device
 * \ingroup plasma
 *
 */
class PLASMA_API usrp_radar : virtual public gr::block
{
public:
    typedef std::shared_ptr<usrp_radar> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::usrp_radar.
     *
     * To avoid accidental use of raw pointers, plasma::usrp_radar's
     * constructor is in a private implementation
     * class. plasma::usrp_radar::make is the public interface for
     * creating new instances.
     */
    static sptr make(double samp_rate,
                     double tx_gain,
                     double rx_gain,
                     double tx_freq,
                     double rx_freq,
                     double tx_start_time,
                     double rx_start_time,
                     const std::string& tx_args,
                     const std::string& rx_args,
                     size_t num_pulse_cpi);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_USRP_RADAR_H */

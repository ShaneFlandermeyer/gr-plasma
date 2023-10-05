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
    static sptr make(const std::string& args, const double tx_rate, const double rx_rate, const double tx_freq, const double rx_freq, const double tx_gain, const double rx_gain, const double start_delay, const bool elevate_priority, const std::string& cal_file, const bool verbose);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_USRP_RADAR_H */

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
    static sptr make(const std::string& args);
    virtual void set_samp_rate(const double) = 0;
    virtual void set_tx_gain(const double) = 0;
    virtual void set_rx_gain(const double) = 0;
    virtual void set_tx_freq(const double) = 0;
    virtual void set_rx_freq(const double) = 0;
    virtual void set_start_time(const double) = 0;
    virtual void set_tx_thread_priority(const double) = 0;
    virtual void set_rx_thread_priority(const double) = 0;
    virtual void read_calibration_file(const std::string&) = 0;
    virtual void set_metadata_keys(std::string center_freq_key,
                                   std::string sample_start_key) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_USRP_RADAR_H */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_USRP_RADAR_IMPL_H
#define INCLUDED_PLASMA_USRP_RADAR_IMPL_H

#include <gnuradio/plasma/usrp_radar.h>
#include <uhd/types/time_spec.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd_radar/receive.h>
#include <uhd_radar/transmit.h>
#include <fstream>

namespace gr {
namespace plasma {

class usrp_radar_impl : public usrp_radar
{
private:
    uhd::usrp::multi_usrp::sptr d_usrp;
    // double d_tx_rate, d_rx_rate;
    double d_samp_rate;
    double d_tx_freq, d_rx_freq;
    double d_tx_gain, d_rx_gain;
    uhd::time_spec_t d_tx_start_time, d_rx_start_time;
    std::string d_tx_args, d_rx_args;
    bool d_finished;
    gr::thread::thread d_thread;

    std::vector<gr_complex> d_data;
    double d_prf;
    double d_num_pulse_cpi;
    size_t d_delay_samps;

public:
    usrp_radar_impl();
    ~usrp_radar_impl();

    void handle_message(const pmt::pmt_t& msg);
    void run();
    bool start() override;
    bool stop() override;

    // Where all the action really happens
    // int work(int noutput_items,
    //          gr_vector_const_void_star& input_items,
    //          gr_vector_void_star& output_items);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_USRP_RADAR_IMPL_H */

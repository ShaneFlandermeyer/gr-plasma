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
#include <uhd/utils/thread.hpp>
#include <fstream>

namespace gr {
namespace plasma {

class usrp_radar_impl : public usrp_radar
{
private:
    uhd::usrp::multi_usrp::sptr d_usrp;
    // double d_tx_rate, d_rx_rate;
    double d_samp_rate;
    double d_tx_gain;
    double d_rx_gain;
    double d_tx_freq;
    double d_rx_freq;
    uhd::time_spec_t d_tx_start_time;
    uhd::time_spec_t d_rx_start_time;
    std::string d_tx_args, d_rx_args;
    std::atomic<bool> d_finished;
    gr::thread::thread d_main_thread;
    gr::thread::thread d_pdu_thread;
    gr::thread::thread d_tx_thread;
    gr::thread::thread d_rx_thread;
    // boost::thread_group d_tx_thread;

    std::vector<gr_complex> d_data;
    std::vector<gr_complex> d_rx_buff;
    double d_prf;
    double d_num_pulse_cpi;
    size_t d_delay_samps;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_pdu_data;

public:
    usrp_radar_impl(double samp_rate,
                    double tx_gain,
                    double rx_gain,
                    double tx_freq,
                    double rx_freq,
                    double tx_start_time,
                    double rx_start_time,
                    const std::string& tx_args,
                    const std::string& rx_args,
                    size_t num_pulse_cpi);
    ~usrp_radar_impl();

    void handle_message(const pmt::pmt_t& msg);
    void send_pdu(std::vector<gr_complex> data);
    void transmit(uhd::usrp::multi_usrp::sptr usrp,
              std::vector<std::complex<float> *> buff_ptrs,
              size_t num_samps_pulse, uhd::time_spec_t start_time);
    void receive(uhd::usrp::multi_usrp::sptr usrp,
                 std::vector<std::complex<float>*> buff_ptrs,
                 size_t num_samps,
                 uhd::time_spec_t start_time);
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

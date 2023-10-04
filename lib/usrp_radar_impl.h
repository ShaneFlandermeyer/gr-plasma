/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_USRP_RADAR_IMPL_H
#define INCLUDED_PLASMA_USRP_RADAR_IMPL_H

#include <gnuradio/plasma/pmt_constants.h>
#include <gnuradio/plasma/usrp_radar.h>
#include <nlohmann/json.hpp>
#include <uhd/convert.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <queue>


namespace gr {
namespace plasma {
static const double BURST_MODE_DELAY = 2e-6;
// static const double BURST_MODE_DELAY = 0;
class usrp_radar_impl : public usrp_radar
{
private:
    std::string d_args;
    double d_tx_rate, d_rx_rate;
    double d_tx_freq, d_rx_freq;
    double d_tx_gain, d_rx_gain;
    std::string d_tx_subdev, d_rx_subdev;
    bool d_elevate_priority;
    

private:
    uhd::usrp::multi_usrp::sptr d_usrp;
    double d_tx_thread_priority;
    double d_rx_thread_priority;
    size_t d_n_samp_pri;
    size_t d_delay_samps;
    size_t d_pulse_count;
    size_t d_tx_sample_count;
    size_t d_rx_sample_count;
    double d_start_time;
    std::vector<gr_complex> d_tx_buff;

    pmt::pmt_t d_meta;
    // Metadata keys
    pmt::pmt_t d_center_freq_key;
    pmt::pmt_t d_sample_start_key;

    pmt::pmt_t rx_data_pmt;
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_out_port;

    gr::thread::thread d_main_thread;
    gr::thread::mutex d_tx_buff_mutex;
    std::atomic<bool> d_finished;
    std::atomic<bool> d_armed;
    boost::thread_group d_tx_rx_thread_group;

    void config_usrp(uhd::usrp::multi_usrp::sptr& usrp,
                     const std::string& args,
                     const double tx_rate,
                     const double rx_rate,
                     const double tx_freq,
                     const double rx_freq,
                     const double tx_gain,
                     const double rx_gain,
                     const std::string& tx_subdev,
                     const std::string& rx_subdev,
                     bool verbose);
    void receive(uhd::usrp::multi_usrp::sptr usrp,
                 uhd::rx_streamer::sptr rx_stream,
                 std::atomic<bool>& finished,
                 bool elevate_priority,
                 double adjusted_rx_delay,
                 bool rx_stream_now);
    void transmit(uhd::usrp::multi_usrp::sptr usrp,
                  uhd::tx_streamer::sptr tx_stream,
                  std::vector<void*> buffs,
                  size_t buff_size,
                  std::atomic<bool>& finished,
                  bool elevate_priority,
                  double tx_delay);

public:
    usrp_radar_impl(const std::string& args);
    ~usrp_radar_impl();

    /**
     * @brief
     *
     * @param msg
     */
    void handle_message(const pmt::pmt_t& msg);

    /**
     * @brief Initialize all buffers and set up the transmit and recieve threads
     *
     */
    void run();
    /**
     * @brief Start the main worker thread
     */
    bool start() override;
    /**
     * @brief Stop the main worker thread
     */
    bool stop() override;

    /**
     * @brief Use the calibration file to determine the number of
     * samples to remove from the beginning of transmission
     *
     */
    // void read_calibration_file(const std::string&) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_USRP_RADAR_IMPL_H */

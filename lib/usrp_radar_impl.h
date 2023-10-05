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
class usrp_radar_impl : public usrp_radar
{
private:
    uhd::usrp::multi_usrp::sptr usrp;
    std::string usrp_args;
    double tx_rate, rx_rate;
    double tx_freq, rx_freq;
    double tx_gain, rx_gain;
    double start_delay;
    bool elevate_priority;
    std::string cal_file;
    std::vector<size_t> tx_channel_nums, rx_channel_nums;
    std::string tx_subdev, rx_subdev;
    std::string tx_device_addr, rx_device_addr;
    std::string tx_cpu_format, rx_cpu_format;
    std::string tx_otw_format, rx_otw_format;
    bool verbose;
    

    gr::thread::thread d_main_thread;
    boost::thread_group d_tx_rx_thread_group;
    std::vector<void*> tx_buffs;
    size_t tx_buff_size;
    std::atomic<bool> finished;
    std::atomic<bool> msg_received;
    size_t n_delay;


private:
    void handle_msg(pmt::pmt_t msg);
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
                  std::atomic<bool>& finished,
                  bool elevate_priority,
                  double tx_delay,
                  double has_time_spec);
    void read_calibration_file(const std::string& filename);

public:
    usrp_radar_impl(const std::string& args,
                    const double tx_rate,
                    const double rx_rate,
                    const double tx_freq,
                    const double rx_freq,
                    const double tx_gain,
                    const double rx_gain,
                    const double start_delay,
                    const bool elevate_priority,
                    const std::string& cal_file,
                    const bool verbose);
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
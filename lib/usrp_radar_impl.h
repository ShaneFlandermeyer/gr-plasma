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
    // Block params
    uhd::usrp::multi_usrp::sptr usrp;
    std::string usrp_args;
    double tx_rate, rx_rate;
    double tx_freq, rx_freq;
    double tx_gain, rx_gain;
    double start_delay;
    std::vector<size_t> tx_channel_nums, rx_channel_nums;
    std::string tx_subdev, rx_subdev;
    std::string tx_device_addr, rx_device_addr;
    std::string tx_cpu_format, rx_cpu_format;
    std::string tx_otw_format, rx_otw_format;
    double prf;

    bool elevate_priority;
    std::string calibration_file;
    bool verbose;
    size_t n_delay;

    // Implementation params
    gr::thread::thread main_thread;
    gr::thread::thread tx_thread;
    gr::thread::thread rx_thread;
    std::vector<std::vector<gr_complex>> tx_buffs;
    std::atomic<bool> finished;
    std::atomic<bool> new_msg_received;
    size_t tx_buff_size, rx_buff_size;
    size_t n_tx_total;

    pmt::pmt_t tx_data;
    pmt::pmt_t meta;

    // Metadata keys
    std::string tx_freq_key;
    std::string rx_freq_key;
    std::string sample_start_key;
    std::string prf_key;


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
                 double start_time);
    void transmit_bursts(uhd::usrp::multi_usrp::sptr usrp,
                         uhd::tx_streamer::sptr tx_stream,
                         double start_time);
    void transmit_continuous(uhd::usrp::multi_usrp::sptr usrp,
                             uhd::tx_streamer::sptr tx_stream,
                             double start_time);
    void read_calibration_file(const std::string& filename);
    void set_metadata_keys(const std::string& tx_freq_key,
                           const std::string& rx_freq_key,
                           const std::string& sample_start_key,
                           const std::string& prf_key);

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

    void run();
    bool start() override;
    bool stop() override;
    void handle_message(const pmt::pmt_t& msg);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_USRP_RADAR_IMPL_H */
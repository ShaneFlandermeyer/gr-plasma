/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "usrp_radar_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

usrp_radar::sptr usrp_radar::make(const std::string& args)
{
    return gnuradio::make_block_sptr<usrp_radar_impl>(args);
}

usrp_radar_impl::usrp_radar_impl(const std::string& args)
    : gr::block(
          "usrp_radar", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0))
{
    // Parameters
    this->usrp_args = args;
    this->tx_rate = 200e6;
    this->rx_rate = 200e6;
    this->tx_freq = 1e9;
    this->rx_freq = 1e9;
    this->tx_gain = 10;
    this->rx_gain = 10;
    this->elevate_priority = true;
    this->tx_subdev = "";
    this->rx_subdev = "";
    config_usrp(this->usrp,
                this->usrp_args,
                this->tx_rate,
                this->rx_rate,
                this->tx_freq,
                this->rx_freq,
                this->tx_gain,
                this->rx_gain,
                this->tx_subdev,
                this->rx_subdev,
                true);

    message_port_register_in(PMT_IN);
    message_port_register_out(PMT_OUT);
    set_msg_handler(PMT_IN, [this](pmt::pmt_t msg) { this->handle_message(msg); });
}

usrp_radar_impl::~usrp_radar_impl() {}


bool usrp_radar_impl::start()
{
    finished = false;
    d_main_thread = gr::thread::thread([this] { run(); });
    return block::start();
}

bool usrp_radar_impl::stop()
{
    finished = true;
    return block::stop();
}

void usrp_radar_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t meta = pmt::car(msg);
        pmt::pmt_t data = pmt::cdr(msg);
        tx_buff_size = pmt::length(data);
        tx_buff = pmt::c32vector_writable_elements(data, tx_buff_size);
        msg_received = true;
    }
}

void usrp_radar_impl::run()
{
    while (not msg_received) {
        if (finished) {
            return;
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    std::atomic<bool>& finished = this->finished;
    if (this->elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    /***********************************************************************
     * Receive thread
     **********************************************************************/
    // TODO: Don't hard-code these
    std::vector<size_t> rx_channel_nums(1, 0);
    bool rx_stream_now = false;
    std::string rx_cpu = "fc32";
    std::string rx_otw = "sc16";
    std::string rx_stream_args = "";

    double start_delay = 0.1;
    double start_time = usrp->get_time_now().get_real_secs() + start_delay;
    if (start_delay == 0.0 && (rx_channel_nums.size() == 1)) {
        rx_stream_now = true;
    }
    // create a receive streamer
    uhd::stream_args_t stream_args(rx_cpu, rx_otw);
    stream_args.channels = rx_channel_nums;
    stream_args.args = uhd::device_addr_t(rx_stream_args);
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);

    auto rx_thread = d_tx_rx_thread_group.create_thread([=, &finished]() {
        receive(usrp, rx_stream, finished, elevate_priority, start_time, rx_stream_now);
    });
    uhd::set_thread_name(rx_thread, "rx_stream");
    /***********************************************************************
     * Transmit thread
     **********************************************************************/
    // TODO: Don't hard-code these
    double tx_delay = 0.1;
    double adjusted_tx_delay = tx_delay;
    std::vector<size_t> tx_channel_nums(1, 0);
    std::string tx_cpu = "fc32";
    std::string tx_otw = "sc16";
    std::string tx_stream_args = "";
    std::vector<void*> tx_buffs;


    // create a transmit streamer
    stream_args.channels = tx_channel_nums;
    stream_args.args = uhd::device_addr_t(tx_stream_args);
    uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        tx_buffs.push_back(this->tx_buff); // same buffer for each channel
    auto tx_thread = d_tx_rx_thread_group.create_thread([=, &finished]() {
        transmit(usrp, tx_stream, tx_buffs, finished, elevate_priority, start_time);
    });
    uhd::set_thread_name(tx_thread, "tx_stream");

    d_tx_rx_thread_group.join_all();
}

void usrp_radar_impl::config_usrp(uhd::usrp::multi_usrp::sptr& usrp,
                                  const std::string& args,
                                  const double tx_rate,
                                  const double rx_rate,
                                  const double tx_freq,
                                  const double rx_freq,
                                  const double tx_gain,
                                  const double rx_gain,
                                  const std::string& tx_subdev,
                                  const std::string& rx_subdev,
                                  bool verbose)
{
    usrp = uhd::usrp::multi_usrp::make(args);
    if (not tx_subdev.empty()) {
        usrp->set_tx_subdev_spec(tx_subdev);
    }
    if (not rx_subdev.empty()) {
        usrp->set_rx_subdev_spec(rx_subdev);
    }
    usrp->set_tx_rate(tx_rate);
    usrp->set_rx_rate(rx_rate);
    usrp->set_tx_freq(tx_freq);
    usrp->set_rx_freq(rx_freq);
    usrp->set_tx_gain(tx_gain);
    usrp->set_rx_gain(rx_gain);

    if (verbose) {
        std::cout << boost::format("Using Device: %s") % usrp->get_pp_string()
                  << std::endl;
        std::cout << boost::format("Actual TX Rate: %f Msps") %
                         (usrp->get_tx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Rate: %f Msps") %
                         (usrp->get_rx_rate() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Freq: %f MHz") % (usrp->get_tx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual RX Freq: %f MHz") % (usrp->get_rx_freq() / 1e6)
                  << std::endl;
        std::cout << boost::format("Actual TX Gain: %f dB") % usrp->get_tx_gain()
                  << std::endl;
        std::cout << boost::format("Actual RX Gain: %f dB") % usrp->get_rx_gain()
                  << std::endl;
    }
}

void usrp_radar_impl::receive(uhd::usrp::multi_usrp::sptr usrp,
                              uhd::rx_streamer::sptr rx_stream,
                              std::atomic<bool>& finished,
                              bool elevate_priority,
                              double start_time,
                              bool rx_stream_now)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe(1, true);
    }

    // setup variables
    uhd::rx_metadata_t md;
    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.num_samps = rx_stream->get_max_num_samps();
    cmd.time_spec = uhd::time_spec_t(start_time);
    cmd.stream_now = rx_stream_now;
    rx_stream->issue_stream_cmd(cmd);

    // Set up and allocate buffers
    std::vector<gr_complex> buff = std::vector<gr_complex>(cmd.num_samps);
    std::vector<void*> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front());
    pmt::pmt_t rx_data_pmt = pmt::make_c32vector(tx_buff_size, 0);
    gr_complex* rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, tx_buff_size);


    double time_until_start = start_time - usrp->get_time_now().get_real_secs();
    double recv_timeout = 0.1 + time_until_start;
    bool stop_called = false;

    // TODO: Handle multiple channels (e.g., one out port per channel)
    // TODO: Hard-coded
    n_delay = 118;
    while (true) {
        if (finished and not stop_called) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            stop_called = true;
        }
        try {
            
            while (n_delay > 0) {
                size_t n_rx = rx_stream->recv(rx_data_ptr,
                                n_delay,
                                md,
                                recv_timeout); // throw away samples
                n_delay -= n_rx;
            }
            rx_stream->recv(rx_data_ptr, tx_buff_size, md, recv_timeout);
            recv_timeout = 0.1;
            message_port_pub(PMT_OUT, pmt::cons(pmt::make_dict(), rx_data_pmt));
        } catch (uhd::io_error& e) {
            std::cerr << "Caught an IO exception. " << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        // Handle errors
        switch (md.error_code) {
        case uhd::rx_metadata_t::ERROR_CODE_NONE:
            if ((finished or stop_called) and md.end_of_burst) {
                return;
            }
            break;
        default:
            break;
        }
    }
}

void usrp_radar_impl::transmit(uhd::usrp::multi_usrp::sptr usrp,
                               uhd::tx_streamer::sptr tx_stream,
                               std::vector<void*> buffs,
                               std::atomic<bool>& finished,
                               bool elevate_priority,
                               double start_time)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe(1, true);
    }

    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = (start_time != 0.0);
    md.time_spec = uhd::time_spec_t(start_time);

    double timeout = 0.1 + start_time;
    while (not finished) {
        const size_t n_sent = tx_stream->send(buffs, tx_buff_size, md, timeout) *
                              tx_stream->get_num_channels();
        md.has_time_spec = false;
        timeout = 0.1;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
}
} /* namespace plasma */
} /* namespace gr */

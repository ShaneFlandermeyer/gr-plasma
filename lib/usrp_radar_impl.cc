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
    d_args = args;
    d_tx_rate = 200e6;
    d_rx_rate = 200e6;
    d_tx_freq = 1e9;
    d_rx_freq = 1e9;
    d_tx_gain = 20;
    d_rx_gain = 20;
    d_elevate_priority = true;
    std::string tx_subdev = "";
    std::string rx_subdev = "";
    config_usrp(d_usrp,
                d_args,
                d_tx_rate,
                d_rx_rate,
                d_tx_freq,
                d_rx_freq,
                d_tx_gain,
                d_rx_gain,
                tx_subdev,
                rx_subdev,
                true);

    message_port_register_in(PMT_IN);
    message_port_register_out(PMT_OUT);
}

usrp_radar_impl::~usrp_radar_impl() {}


bool usrp_radar_impl::start()
{
    d_finished = false;
    d_main_thread = gr::thread::thread([this] { run(); });
    return block::start();
}

bool usrp_radar_impl::stop()
{
    d_finished = true;
    return block::stop();
}

void usrp_radar_impl::run()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::atomic<bool>& finished = d_finished;
    if (d_elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    /***********************************************************************
     * Receive thread
     **********************************************************************/
    // TODO: Don't hard-code these
    double rx_delay = 0;
    double adjusted_rx_delay = rx_delay;
    std::vector<size_t> rx_channel_nums(1, 0);
    bool rx_stream_now = false;
    std::string rx_cpu = "fc32";
    std::string rx_otw = "sc16";
    std::string rx_stream_args = "";

    if (rx_delay == 0.0 && rx_channel_nums.size() > 1) {
        adjusted_rx_delay = std::max(rx_delay, 0.05);
    }
    if (rx_delay == 0.0 && (rx_channel_nums.size() == 1)) {
        rx_stream_now = true;
    }
    d_usrp->set_time_now(0.0);
    // create a receive streamer
    uhd::stream_args_t stream_args(rx_cpu, rx_otw);
    stream_args.channels = rx_channel_nums;
    stream_args.args = uhd::device_addr_t(rx_stream_args);
    uhd::rx_streamer::sptr rx_stream = d_usrp->get_rx_stream(stream_args);
    
    auto rx_thread = d_tx_rx_thread_group.create_thread([=, &finished]() {
        receive(d_usrp,
                rx_stream,
                finished,
                d_elevate_priority,
                adjusted_rx_delay,
                rx_stream_now);
    });
    uhd::set_thread_name(rx_thread, "rx_stream");
    /***********************************************************************
     * Transmit thread
     **********************************************************************/
    double tx_delay = 0;
    double adjusted_tx_delay = tx_delay;
    std::vector<size_t> tx_channel_nums(1, 0);
    std::string tx_cpu = "fc32";
    std::string tx_otw = "sc16";
    std::string tx_stream_args = "";
    std::vector<gr_complex> tx_buff;
    std::vector<void*> tx_buffs;

    if (tx_delay == 0.0 && tx_channel_nums.size() > 1) {
        adjusted_tx_delay = std::max(tx_delay, 0.25);
    }

    // create a transmit streamer
    // uhd::stream_args_t stream_args(tx_cpu, tx_otw);
    stream_args.channels = tx_channel_nums;
    stream_args.args = uhd::device_addr_t(tx_stream_args);
    uhd::tx_streamer::sptr tx_stream = d_usrp->get_tx_stream(stream_args);
    size_t tx_buff_size = tx_stream->get_max_num_samps();
    tx_buff = std::vector<gr_complex>(tx_buff_size);
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        tx_buffs.push_back(&tx_buff.front()); // same buffer for each channel
    auto tx_thread = d_tx_rx_thread_group.create_thread([=, &finished]() {
        transmit(d_usrp,
                 tx_stream,
                 tx_buffs,
                 tx_buff_size,
                 finished,
                 d_elevate_priority,
                 adjusted_tx_delay);
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
                              double adjusted_rx_delay,
                              bool rx_stream_now)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe(1, true);
    }

    // setup variables
    uhd::rx_metadata_t md;
    uhd::stream_cmd_t cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    cmd.num_samps = rx_stream->get_max_num_samps();
    cmd.time_spec = usrp->get_time_now() + uhd::time_spec_t(adjusted_rx_delay);
    cmd.stream_now = rx_stream_now;
    rx_stream->issue_stream_cmd(cmd);

    // Set up and allocate buffers
    size_t buff_size = rx_stream->get_max_num_samps();
    std::vector<gr_complex> buff = std::vector<gr_complex>(buff_size);
    std::vector<void*> buffs;
    for (size_t ch = 0; ch < rx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front());
    // TODO: PMT size should be one PRI
    pmt::pmt_t rx_data_pmt = pmt::make_c32vector(buff_size, 0);
    gr_complex* rx_data_ptr = pmt::c32vector_writable_elements(rx_data_pmt, buff_size);

    float recv_timeout = 0.1 + adjusted_rx_delay;
    bool stop_called = false;
    while (true) {
        if (finished and not stop_called) {
            rx_stream->issue_stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
            stop_called = true;
        }
        try {
            size_t n_recv_samps =
                rx_stream->recv(buffs, cmd.num_samps, md, recv_timeout) *
                rx_stream->get_num_channels();
            recv_timeout = 0.1;
            // Copy result to PDU output
            // TODO: Make this a function
            // TODO: Account for constant delay
            size_t n_samps_remaining = n_recv_samps;
            size_t n_samps_written = 0;
            size_t start = 0;
            size_t stop = std::min(n_recv_samps, start + buff_size);
            while (n_samps_remaining > 0) {
                std::copy((gr_complex*)buffs[0] + start,
                          (gr_complex*)buffs[0] + stop,
                          rx_data_ptr + n_samps_written);
                n_samps_written += stop - start;
                n_samps_remaining -= stop - start;
                if (n_samps_written == buff_size) {
                    message_port_pub(PMT_OUT, pmt::cons(pmt::make_dict(), rx_data_pmt));
                    n_samps_written = 0;
                }
                start = stop;
                stop = std::min(n_recv_samps, start + buff_size - n_samps_written);
            }
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
                               size_t buff_size,
                               std::atomic<bool>& finished,
                               bool elevate_priority,
                               double tx_delay)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe(1, true);
    }

    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = (tx_delay != 0.0);
    md.time_spec = usrp->get_time_now() + uhd::time_spec_t(tx_delay);

    double timeout = 0.1 + tx_delay;
    while (not finished) {
        const size_t n_sent = tx_stream->send(buffs, buff_size, md, timeout) *
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

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
          "usrp_radar", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_args(args),
      d_samp_rate(200e6),
      d_center_freq(5e9)
{
    std::string cpu_format = "fc32";
    std::string tx_otw = "sc16";
    std::string tx_stream_args = "";

    d_usrp = uhd::usrp::multi_usrp::make(args);


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
    boost::thread_group thread_group;

    d_usrp->set_tx_rate(d_samp_rate);

    uhd::stream_args_t stream_args("fc32", "sc16");
    stream_args.channels = std::vector<size_t>(1,0);
    stream_args.args = uhd::device_addr_t("");
    uhd::tx_streamer::sptr tx_stream = d_usrp->get_tx_stream(stream_args);
    const size_t max_spp = tx_stream->get_max_num_samps();
    size_t spp = max_spp;
    size_t spb = spp;
    bool elevate_priority = false;
    double adjusted_tx_delay = 0.;
    std::atomic<bool> burst_timer_elapsed(false);
    auto tx_thread = thread_group.create_thread([=]() {
        transmit(d_usrp,
                 tx_stream,
                 spb,
                 elevate_priority,
                 adjusted_tx_delay);
    });
    uhd::set_thread_name(tx_thread, "tx_stream");

    thread_group.join_all();
}

void usrp_radar_impl::transmit(uhd::usrp::multi_usrp::sptr usrp,
                               uhd::tx_streamer::sptr tx_stream,
                            //    std::atomic<bool>& burst_timer_elapsed,
                               const size_t spb,
                               bool elevate_priority,
                               double tx_delay,
                               const std::string& cpu_format)
{
    if (elevate_priority) {
        uhd::set_thread_priority_safe();
    }

    // setup variables and allocate buffer
    // TODO: Set up buffers using message port
    std::vector<char> buff(spb * uhd::convert::get_bytes_per_item(cpu_format));
    std::vector<const void*> buffs;
    for (size_t ch = 0; ch < tx_stream->get_num_channels(); ch++)
        buffs.push_back(&buff.front()); // same buffer for each channel
    // Create the metadata, and populate the time spec at the latest possible moment
    uhd::tx_metadata_t md;
    md.has_time_spec = (tx_delay != 0.0);
    md.time_spec = usrp->get_time_now() + uhd::time_spec_t(tx_delay);
    double timeout = 0.1 + tx_delay;

    while (not d_finished) {
        const size_t n_sent =
            tx_stream->send(buffs, spb, md, timeout) * tx_stream->get_num_channels();
        md.has_time_spec = false;
        timeout = 0.1;
    }

    // send a mini EOB packet
    md.end_of_burst = true;
    tx_stream->send(buffs, 0, md);
}

} /* namespace plasma */
} /* namespace gr */

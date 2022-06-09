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

// std::vector<gr_complex> rx_buff;
usrp_radar::sptr usrp_radar::make(double samp_rate,
                                  double tx_gain,
                                  double rx_gain,
                                  double tx_freq,
                                  double rx_freq,
                                  double tx_start_time,
                                  double rx_start_time,
                                  const std::string& tx_args,
                                  const std::string& rx_args,
                                  size_t num_pulse_cpi)
{
    return gnuradio::make_block_sptr<usrp_radar_impl>(samp_rate,
                                                      tx_gain,
                                                      rx_gain,
                                                      tx_freq,
                                                      rx_freq,
                                                      tx_start_time,
                                                      rx_start_time,
                                                      tx_args,
                                                      rx_args,
                                                      num_pulse_cpi);
}


/*
 * The private constructor
 */
usrp_radar_impl::usrp_radar_impl(double samp_rate,
                                 double tx_gain,
                                 double rx_gain,
                                 double tx_freq,
                                 double rx_freq,
                                 double tx_start_time,
                                 double rx_start_time,
                                 const std::string& tx_args,
                                 const std::string& rx_args,
                                 size_t num_pulse_cpi)
    : gr::block(
          "usrp_radar", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_samp_rate(samp_rate),
      d_tx_gain(tx_gain),
      d_rx_gain(rx_gain),
      d_tx_freq(tx_freq),
      d_rx_freq(rx_freq),
      d_tx_start_time(tx_start_time),
      d_rx_start_time(rx_start_time),
      d_tx_args(tx_args),
      d_rx_args(rx_args),
      d_num_pulse_cpi(num_pulse_cpi)
{
    message_port_register_in(pmt::mp("in"));
    message_port_register_out(pmt::mp("out"));
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    // Initialize the USRP device
    d_usrp = uhd::usrp::multi_usrp::make(d_tx_args);
    d_usrp->set_tx_rate(d_samp_rate);
    d_usrp->set_rx_rate(d_samp_rate);
    d_usrp->set_tx_freq(d_tx_freq);
    d_usrp->set_rx_freq(d_rx_freq);
    d_usrp->set_tx_gain(d_tx_gain);
    d_usrp->set_rx_gain(d_rx_gain);
}

/*
 * Our virtual destructor.
 */
usrp_radar_impl::~usrp_radar_impl() {}

void usrp_radar_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        // Parse the PDU and store the data in a member variable
        d_meta = pmt::car(msg);
        // Append additional metadata to the pmt object
        d_meta = pmt::dict_add(
            d_meta, pmt::intern("num_pulse_cpi"), pmt::from_long(d_num_pulse_cpi));
        pmt::pmt_t data = pmt::cdr(msg);
        d_data = c32vector_elements(data);
    }
}

// std::vector<gr_complex> big_boi;
inline void usrp_radar_impl::send_pdu(std::vector<gr_complex> data)
{
    uhd::set_thread_priority_safe();
    d_pdu_data = pmt::init_c32vector(data.size(), data.data());
    pmt::pmt_t pdu = pmt::cons(d_meta, d_pdu_data);
    message_port_pub(pmt::mp("out"), pdu);

    // std::ofstream outfile("/home/shane/test.dat",
    //                       std::ios::out | std::ios::binary | std::ios::app);
    // outfile.write(reinterpret_cast<char*>(data.data()), sizeof(gr_complex) *
    // data.size()); outfile.close();
}

void usrp_radar_impl::transmit(uhd::usrp::multi_usrp::sptr usrp,
                               std::vector<std::complex<float>*> buff_ptrs,
                               size_t num_samps_pulse,
                               uhd::time_spec_t start_time)
{
    uhd::set_thread_priority_safe(1);
    uhd::stream_args_t tx_stream_args("fc32", "sc16");
    uhd::tx_streamer::sptr tx_stream;
    tx_stream_args.channels.push_back(0);
    tx_stream = usrp->get_tx_stream(tx_stream_args);
    // Create metadata structure
    uhd::tx_metadata_t tx_md;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst = false;
    tx_md.has_time_spec = (start_time.get_real_secs() > 0 ? true : false);
    tx_md.time_spec = start_time;

    while (!d_finished) {
        tx_stream->send(buff_ptrs, num_samps_pulse, tx_md, 0.5);
        tx_md.start_of_burst = false;
        tx_md.has_time_spec = false;
    }
    tx_md.has_time_spec = false;
    tx_md.end_of_burst = true;
    tx_stream->send("", 0, tx_md);
}

void usrp_radar_impl::receive(uhd::usrp::multi_usrp::sptr usrp,
                              std::vector<gr_complex*> buff_ptrs,
                              size_t num_samps_cpi,
                              uhd::time_spec_t start_time)
{
    uhd::set_thread_priority_safe(1);
    size_t channels = buff_ptrs.size();
    std::vector<size_t> channel_vec;
    uhd::stream_args_t stream_args("fc32", "sc16");
    uhd::rx_streamer::sptr rx_stream;
    if (channel_vec.size() == 0) {
        for (size_t i = 0; i < channels; i++) {
            channel_vec.push_back(i);
        }
    }
    stream_args.channels = channel_vec;
    rx_stream = usrp->get_rx_stream(stream_args);

    uhd::rx_metadata_t md;
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    stream_cmd.num_samps = num_samps_cpi;
    stream_cmd.time_spec = start_time;
    stream_cmd.stream_now = (start_time.get_real_secs() > 0 ? false : true);
    rx_stream->issue_stream_cmd(stream_cmd);

    size_t max_num_samps = rx_stream->get_max_num_samps();
    size_t num_samps_total = 0;
    std::vector<gr_complex*> buff_ptrs2(buff_ptrs.size());
    double timeout = 0.5 + start_time.get_real_secs();
    while (!d_finished) {
        // Move storing pointer to correct location
        for (size_t i = 0; i < channels; i++)
            buff_ptrs2[i] = &(buff_ptrs[i][num_samps_total]);

        // Sampling data
        size_t samps_to_recv = std::min(num_samps_cpi - num_samps_total, max_num_samps);
        size_t num_rx_samps = rx_stream->recv(buff_ptrs2, samps_to_recv, md, timeout);

        timeout = 0.5;

        num_samps_total += num_rx_samps;

        // TODO: Handle errors
        // Send the pdu
        if (num_samps_total >= num_samps_cpi) {
            d_pdu_thread.join();
            d_pdu_thread = gr::thread::thread([this] { send_pdu(d_rx_buff); });
            num_samps_total = 0;
        }
    }
    stream_cmd.stream_mode = uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS;
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);
}

bool usrp_radar_impl::start()
{
    d_finished = false;
    d_main_thread = gr::thread::thread([this] { run(); });

    return block::start();
}

bool usrp_radar_impl::stop()
{
    d_finished = true;
    d_main_thread.join();

    return block::stop();
}

void usrp_radar_impl::run()
{
    uhd::set_thread_priority_safe();
    // boost::thread_group tx_thread;
    while (d_data.size() == 0) {
        // Wait for data to arrive
        boost::this_thread::sleep(boost::posix_time::microseconds(1));
    }
    // Set up Tx buffer
    std::vector<gr_complex*> tx_buff_ptrs;
    tx_buff_ptrs.push_back(&d_data.front());

    // Set up Rx buffer
    size_t num_samp_rx = d_data.size() * d_num_pulse_cpi;
    std::vector<gr_complex*> rx_buff_ptrs;
    d_rx_buff = std::vector<gr_complex>(num_samp_rx, 0);
    rx_buff_ptrs.push_back(&d_rx_buff.front());

    // Start the transmit and receive threads
    uhd::time_spec_t time_now = d_usrp->get_time_now();
    d_tx_thread = gr::thread::thread([this, tx_buff_ptrs, time_now] {
        transmit(d_usrp, tx_buff_ptrs, d_data.size(), time_now + d_tx_start_time);
    });
    d_rx_thread = gr::thread::thread([this, rx_buff_ptrs, num_samp_rx, time_now] {
        receive(d_usrp, rx_buff_ptrs, num_samp_rx, time_now + d_rx_start_time);
    });

    d_tx_thread.join();
    d_rx_thread.join();
}

} /* namespace plasma */
} /* namespace gr */

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

usrp_radar::sptr usrp_radar::make()
{
    return gnuradio::make_block_sptr<usrp_radar_impl>();
}


/*
 * The private constructor
 */
usrp_radar_impl::usrp_radar_impl()
    : gr::block(
          "usrp_radar", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0))
{
    message_port_register_in(pmt::mp("in"));
    message_port_register_out(pmt::mp("out"));
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
// Initialize the USRP device
#pragma message("TODO: Don't hard-code these parameters")
    d_samp_rate = 30e6;
    d_tx_gain = 50;
    d_rx_gain = 40;
    d_tx_freq = 5e9;
    d_rx_freq = 5e9;
    d_tx_start_time = 0.2;
    d_rx_start_time = 0.2;
    d_tx_args = "";
    d_rx_args = "";
    d_num_pulse_cpi = 1024;
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
        pmt::pmt_t meta = pmt::car(msg);
        pmt::pmt_t data = pmt::cdr(msg);
        size_t num_samp_pulse = pmt::length(data);
        size_t zero(0);
        gr_complex* ptr = (gr_complex*)pmt::uniform_vector_writable_elements(data, zero);
        d_data = std::vector<gr_complex>(ptr, ptr + num_samp_pulse);
    }
}

bool usrp_radar_impl::start()
{
    d_finished = false;
    d_thread = gr::thread::thread([this] { run(); });

    return block::start();
}

bool usrp_radar_impl::stop()
{
    d_finished = true;
    d_thread.interrupt();
    d_thread.join();

    return block::stop();
}

void usrp_radar_impl::run()
{
#pragma message("TODO: Implement the radar and remove this warning")
    boost::thread_group tx_thread;
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
    std::vector<gr_complex> rx_buff(num_samp_rx, 0);
    rx_buff_ptrs.push_back(&rx_buff.front());

    // Simultaneously transmit and receive the data
    uhd::time_spec_t time_now = d_usrp->get_time_now();
    tx_thread.create_thread(boost::bind(&uhd::radar::transmit,
                                        d_usrp,
                                        tx_buff_ptrs,
                                        d_num_pulse_cpi,
                                        d_data.size(),
                                        time_now + d_tx_start_time));
    size_t n = uhd::radar::receive(
        d_usrp, rx_buff_ptrs, num_samp_rx, time_now + d_rx_start_time);
    tx_thread.join_all();

#pragma message("TODO: Remove file write")
    std::ofstream outfile("/home/shane/gnuradar_test.dat",
                          std::ios::out | std::ios::binary);
    outfile.write((char*)rx_buff.data(), rx_buff.size() * sizeof(gr_complex));
    outfile.close();
}

} /* namespace plasma */
} /* namespace gr */

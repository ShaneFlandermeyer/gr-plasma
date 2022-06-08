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

    // Send the pdu
    pmt::pmt_t pdu = pmt::cons(d_meta, pmt::init_c32vector(rx_buff.size(), rx_buff));
    message_port_pub(pmt::mp("out"), pdu);

    // #pragma message("TODO: Remove file write")
    //     std::ofstream outfile("/home/shane/gnuradar_test.dat",
    //                           std::ios::out | std::ios::binary);
    //     outfile.write((char*)rx_buff.data(), rx_buff.size() * sizeof(gr_complex));
    //     outfile.close();
    // GR_LOG_DEBUG(d_logger, "Wrote to file");
}

} /* namespace plasma */
} /* namespace gr */

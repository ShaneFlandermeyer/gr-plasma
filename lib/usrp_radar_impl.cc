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

    // TODO: Derive these from inputs and message metadata
    d_prf = 1e3;
    d_samp_rate = 50e6;
}

/*
 * Our virtual destructor.
 */
usrp_radar_impl::~usrp_radar_impl() {}

void usrp_radar_impl::handle_message(const pmt::pmt_t& msg)
{
#pragma message("TODO: Implement the message handler and remove this warning")
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t meta = pmt::car(msg);
        pmt::pmt_t data = pmt::cdr(msg);
        size_t num_samp_pulse = pmt::length(data);
        size_t zero(0);
        gr_complex* ptr = (gr_complex*)pmt::uniform_vector_writable_elements(data, zero);
        d_data = std::vector<gr_complex>(ptr, ptr + num_samp_pulse);
        // TODO: Insert zeros to get to the PRF length
        GR_LOG_DEBUG(d_logger, d_data.size());
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
}

} /* namespace plasma */
} /* namespace gr */

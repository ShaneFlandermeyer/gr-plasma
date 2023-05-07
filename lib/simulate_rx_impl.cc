/* -*- c++ -*- */
/*
 * Copyright 2023 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "simulate_rx_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

simulate_rx::sptr simulate_rx::make(double delay, double scale)
{
    return gnuradio::make_block_sptr<simulate_rx_impl>(delay, scale);
}


/*
 * The private constructor
 */
simulate_rx_impl::simulate_rx_impl(double delay, double scale)
    : gr::block("simulate_rx",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
        d_in_port(PMT_IN),
        d_out_port(PMT_OUT)
{
    GR_LOG_WARN(d_logger, "This block does not actually simulate anything yet. It is just a placeholder to convert a PDU to a Tx/Rx meta pair.")
    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);
    set_msg_handler(d_in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

void simulate_rx_impl::handle_msg(pmt::pmt_t msg) {
    pmt::pmt_t samples, meta;
    if (pmt::is_pdu(msg)) {
        // Get the transmit data
        samples = pmt::cdr(msg);
        meta = pmt::car(msg);
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Unexpected message type");
        return;
    }

    // Add the input PDU data as tx, output as rx in dict
    // TODO: Allow the user to customize these keys
    meta = pmt::dict_add(meta, pmt::mp("tx"), samples);
    meta = pmt::dict_add(meta, pmt::mp("rx"), samples);
    message_port_pub(d_out_port, meta);
}

} /* namespace plasma */
} /* namespace gr */

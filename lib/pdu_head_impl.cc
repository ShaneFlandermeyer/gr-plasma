/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_head_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

pdu_head::sptr pdu_head::make(size_t nitems)
{
    return gnuradio::make_block_sptr<pdu_head_impl>(nitems);
}

/*
 * Our virtual destructor.
 */
pdu_head_impl::~pdu_head_impl() {}

/*
 * The private constructor
 */
pdu_head_impl::pdu_head_impl(size_t nitems)
    : gr::block(
          "pdu_head", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
          d_nitems(nitems)
{
    // TODO: Declare port names as const in a separate file
    message_port_register_in(PMT_IN);
    message_port_register_out(PMT_OUT);
    set_msg_handler(PMT_IN,
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    d_nitems_sent = 0;
}

void pdu_head_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg) and d_nitems_sent < d_nitems) {
        message_port_pub(PMT_OUT, msg);
        d_nitems_sent++;
    }
    // TODO: Handle messages that are not PDUs
}


} /* namespace plasma */
} /* namespace gr */

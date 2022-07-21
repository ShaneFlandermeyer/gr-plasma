/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pulse_to_cpi_impl.h"
#include <gnuradio/io_signature.h>
#include <chrono>

namespace gr {
namespace plasma {

pulse_to_cpi::sptr pulse_to_cpi::make(size_t num_pulse_cpi)
{
    return gnuradio::make_block_sptr<pulse_to_cpi_impl>(num_pulse_cpi);
}


/*
 * The private constructor
 */
pulse_to_cpi_impl::pulse_to_cpi_impl(size_t num_pulse_cpi)
    : gr::block("pulse_to_cpi",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_num_pulse_cpi(num_pulse_cpi)
{
    d_pulse_count = 0;
    d_in_port = PMT_IN;
    d_out_port = PMT_OUT;
    d_meta = pmt::make_dict();
    d_meta = pmt::dict_add(d_meta, PMT_NUM_PULSE_CPI, pmt::from_long(d_num_pulse_cpi));

    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);

    set_msg_handler(d_in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

/*
 * Our virtual destructor.
 */
pulse_to_cpi_impl::~pulse_to_cpi_impl() {}

void pulse_to_cpi_impl::handle_msg(pmt::pmt_t msg)
{
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        d_meta = pmt::dict_update(d_meta, pmt::car(msg));
        samples = pmt::cdr(msg);
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
    }
    // Store the new PDU data
    std::vector<gr_complex> newdata = pmt::c32vector_elements(samples);
    d_data.insert(d_data.end(), newdata.begin(), newdata.end());
    d_pulse_count++;
    // Output a PDU containing all the pulses in a column-major format
    if (d_pulse_count == d_num_pulse_cpi) {
        message_port_pub(d_out_port,
                         pmt::cons(d_meta, pmt::init_c32vector(d_data.size(), d_data)));
        d_data.clear();
        d_meta = pmt::make_dict();
        d_pulse_count = 0;
    }
}
} /* namespace plasma */
} /* namespace gr */

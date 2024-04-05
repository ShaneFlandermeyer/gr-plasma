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

pulse_to_cpi::sptr pulse_to_cpi::make(size_t n_pulse_cpi)
{
    return gnuradio::make_block_sptr<pulse_to_cpi_impl>(n_pulse_cpi);
}


/*
 * The private constructor
 */
pulse_to_cpi_impl::pulse_to_cpi_impl(size_t n_pulse_cpi)
    : gr::block("pulse_to_cpi",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_n_pulse_cpi(n_pulse_cpi)
{
    d_pulse_count = 0;
    d_in_port = PMT_IN;
    d_out_port = PMT_OUT;


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
        // Update input metadata
        d_meta = pmt::dict_update(d_meta, pmt::car(msg));
        samples = pmt::cdr(msg);
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
    }

    size_t samps_per_pulse = pmt::length(samples);
    if (d_pulse_count == 0) {
        if (d_data.size() != d_n_pulse_cpi * samps_per_pulse) {
            d_data = std::vector<gr_complex>(d_n_pulse_cpi * samps_per_pulse);
        }
    }
    // Store the new PDU data
    const gr_complex* data = pmt::c32vector_elements(samples, samps_per_pulse);
    // Insert the data into the global data vector at the current index
    std::copy(
        data, data + samps_per_pulse, d_data.begin() + d_pulse_count * samps_per_pulse);

    d_pulse_count++;
    // Output a PDU containing all the pulses in a column-major format
    if (d_pulse_count == d_n_pulse_cpi) {
        message_port_pub(d_out_port,
                         pmt::cons(d_meta, pmt::init_c32vector(d_data.size(), d_data)));
        // Reset the metadata
        d_meta = pmt::make_dict();
        d_pulse_count = 0;
    }
}

void pulse_to_cpi_impl::init_meta_dict(std::string n_pulse_cpi_key)
{
    d_n_pulse_cpi_key = pmt::string_to_symbol(n_pulse_cpi_key);
    d_meta = pmt::make_dict();
    d_meta = pmt::dict_add(d_meta, d_n_pulse_cpi_key, pmt::from_long(d_n_pulse_cpi));
}
} /* namespace plasma */
} /* namespace gr */

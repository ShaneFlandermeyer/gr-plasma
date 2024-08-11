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
      pulses_per_cpi(n_pulse_cpi)
{
    pulse_count = 0;
    in_port = PMT_IN;
    out_port = PMT_OUT;


    message_port_register_in(in_port);
    message_port_register_out(out_port);
    set_msg_handler(in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
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
        meta = pmt::dict_update(meta, pmt::car(msg));
        samples = pmt::cdr(msg);
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
    }

    size_t num_samples = pmt::length(samples);
    if (pulse_count == 0) {
        if (data.size() != pulses_per_cpi * num_samples) {
            data = std::vector<gr_complex>(pulses_per_cpi * num_samples);
        }
    }
    // Store the new PDU data
    const gr_complex* samples_ptr = pmt::c32vector_elements(samples, num_samples);
    size_t start = pulse_count * num_samples;
    // Replace the for-loop above with std::copy
    std::copy(samples_ptr, samples_ptr + num_samples, data.begin() + start);

    pulse_count++;
    // Output a PDU containing all the pulses in a column-major format
    if (pulse_count == pulses_per_cpi) {
        message_port_pub(out_port,
                         pmt::cons(meta, pmt::init_c32vector(data.size(), data)));
        // Reset the metadata
        meta = pmt::make_dict();
        pulse_count = 0;
    }
}

void pulse_to_cpi_impl::init_meta_dict(std::string n_pulse_cpi_key)
{
    this->pulses_per_cpi_key = pmt::string_to_symbol(n_pulse_cpi_key);
    meta = pmt::make_dict();
    meta = pmt::dict_add(meta, pulses_per_cpi_key, pmt::from_long(pulses_per_cpi));
}
} /* namespace plasma */
} /* namespace gr */

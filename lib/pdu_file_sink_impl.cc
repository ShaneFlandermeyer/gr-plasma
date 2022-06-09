/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_file_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <uhd/utils/thread.hpp>

namespace gr {
namespace plasma {

// #pragma message("set the following appropriately and remove this warning")
// using input_type = float;
pdu_file_sink::sptr pdu_file_sink::make(size_t num_pulse_cpi,const std::string& filename)
{
    return gnuradio::make_block_sptr<pdu_file_sink_impl>(num_pulse_cpi, filename);
}


/*
 * The private constructor
 */
pdu_file_sink_impl::pdu_file_sink_impl(size_t num_pulse_cpi,const std::string& filename)
    : gr::block("pdu_file_sink",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_num_pulse_cpi(num_pulse_cpi)
{
    message_port_register_in(pmt::mp("in"));
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    d_current_pulse_index = 0;
    d_file =
        std::ofstream("/home/shane/pdu_file_sink.dat", std::ios::binary | std::ios::out);
}

void pdu_file_sink_impl::handle_message(const pmt::pmt_t& msg)
{
    // TODO: Probably better to maintain a PDU queue and do the file write elsewhere
    uhd::set_thread_priority_safe(1);
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t data = pmt::cdr(msg);
        std::vector<gr_complex> pulse = c32vector_elements(data);
        d_file.write(reinterpret_cast<char*>(pulse.data()),
                     sizeof(gr_complex) * pulse.size());
    }
}


/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl() { d_file.close(); }


} /* namespace plasma */
} /* namespace gr */

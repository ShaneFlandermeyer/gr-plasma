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
pdu_file_sink::sptr pdu_file_sink::make(size_t num_pulse_cpi, const std::string& filename)
{
    return gnuradio::make_block_sptr<pdu_file_sink_impl>(num_pulse_cpi, filename);
}


/*
 * The private constructor
 */
pdu_file_sink_impl::pdu_file_sink_impl(size_t num_pulse_cpi, const std::string& filename)
    : gr::block("pdu_file_sink",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_num_pulse_cpi(num_pulse_cpi),
      d_filename(filename)
{
    message_port_register_in(pmt::mp("in"));
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    // d_current_pulse_index = 0;
    d_file = std::ofstream(d_filename, std::ios::binary | std::ios::out);
    d_lock = std::unique_lock<std::mutex>(d_mutex);
}

void pdu_file_sink_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        pmt::pmt_t data = pmt::cdr(msg);
        d_num_samp_pulse = pmt::length(data);
        d_pulse_queue.push(c32vector_elements(pmt::cdr(msg)).data());
    }
}

bool pdu_file_sink_impl::start()
{
    d_finished = false;
    d_thread = gr::thread::thread([this] { run(); });

    return block::start();
}

bool pdu_file_sink_impl::stop()
{
    d_finished = true;
    d_thread.join();

    return block::stop();
}

void pdu_file_sink_impl::run()
{

    while (!d_finished) {
        while (not d_pulse_queue.empty()) {
            d_file.write((char*)d_pulse_queue.front(),
                         sizeof(gr_complex) * d_num_samp_pulse);
            d_pulse_queue.pop();
        }
    }
}


/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl() { d_file.close(); }


} /* namespace plasma */
} /* namespace gr */

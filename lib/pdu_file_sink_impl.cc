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
    d_file = std::ofstream(d_filename, std::ios::binary | std::ios::out);
}

void pdu_file_sink_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        gr::thread::scoped_lock lock(d_mutex);
        d_data_queue.push(pmt::cdr(msg));
        d_meta_queue.push(pmt::car(msg));
        d_cond.notify_one();
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
    d_cond.notify_one();
    d_thread.join();

    return block::stop();
}

void pdu_file_sink_impl::run()
{

    while (true) {
        {
            gr::thread::scoped_lock lock(d_mutex);
            d_cond.wait(lock, [this] { return not d_data_queue.empty() || d_finished; });
            if (d_finished)
                return;
            d_data = d_data_queue.front();
            d_meta = d_meta_queue.front();
            d_data_queue.pop();
            d_meta_queue.pop();
        }
        size_t n = pmt::length(d_data);
        d_file.write((char*)pmt::c32vector_writable_elements(d_data, n),
                     n * sizeof(gr_complex));
    }
}


/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl() { d_file.close(); }


} /* namespace plasma */
} /* namespace gr */

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
pdu_file_sink::sptr pdu_file_sink::make(std::string& data_filename,
                                        std::string& meta_filename)
{
    return gnuradio::make_block_sptr<pdu_file_sink_impl>(data_filename, meta_filename);
}


/*
 * The private constructor
 */
pdu_file_sink_impl::pdu_file_sink_impl(std::string& data_filename,
                                       std::string& meta_filename)
    : gr::block("pdu_file_sink",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_data_filename(data_filename),
      d_meta_filename(meta_filename)
{
    // TODO: Declare port names as const in a separate file
    message_port_register_in(pmt::mp("in"));
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    d_data_file = std::ofstream(d_data_filename, std::ios::binary | std::ios::out);
    if (not d_meta_filename.empty()) {
        d_meta_file = std::ofstream(d_meta_filename, std::ios::out);
    }
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
        d_data_file.write((char*)pmt::c32vector_writable_elements(d_data, n),
                          n * sizeof(gr_complex));
        if (d_meta_file.is_open()) {
            // TODO: Write the metadata to a sigmf file instead
            pmt::pmt_t items = pmt::dict_items(d_meta);
            for (size_t i = 0; i < pmt::length(items); i++) {
                std::string key = pmt::write_string(pmt::car(pmt::nth(i, items)));
                std::string value = pmt::write_string(pmt::cdr(pmt::nth(i, items)));
                std::string line = key + ": " + value + "\n";
                d_meta_file.write(line.c_str(), line.size());
            }
            d_meta_file.write("\n", 1);
        }
    }
}


/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl()
{
    d_data_file.close();
    d_meta_file.close();
}


} /* namespace plasma */
} /* namespace gr */

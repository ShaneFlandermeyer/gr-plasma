/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_file_sink_impl.h"
#include <gnuradio/io_signature.h>
#include <bit>

namespace gr {
namespace plasma {

/**
 * @brief Check if the system is big or little endian for the datatype string
 *
 * Shamelessly stolen from stackoverflow
 *
 * @return true
 * @return false
 */
bool is_big_endian(void)
{
    union {
        uint32_t i;
        char c[4];
    } bint = { 0x01020304 };

    return bint.c[0] == 1;
}


pdu_file_sink::sptr pdu_file_sink::make(size_t itemsize,
                                        std::string& data_filename,
                                        std::string& meta_filename)
{
    return gnuradio::make_block_sptr<pdu_file_sink_impl>(
        itemsize, data_filename, meta_filename);
}


/*
 * The private constructor
 */
pdu_file_sink_impl::pdu_file_sink_impl(size_t itemsize,
                                       std::string& data_filename,
                                       std::string& meta_filename)
    : gr::block("pdu_file_sink",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_itemsize(itemsize),
      d_data_filename(data_filename),
      d_meta_filename(meta_filename)
{
    // TODO: Declare port names as const in a separate file
    message_port_register_in(PMT_IN);
    set_msg_handler(PMT_IN, [this](const pmt::pmt_t& msg) { handle_message(msg); });
    d_data_file = std::ofstream(d_data_filename, std::ios::binary | std::ios::out);
    if (not d_meta_filename.empty()) {
        d_meta_file = std::ofstream(d_meta_filename, std::ios::out);
        d_meta_dict = pmt::make_dict();
        // Initialize the global metadata fields
        pmt::pmt_t global = pmt::make_dict();
        global = pmt::dict_add(
            global, pmt::intern("core:datatype"), pmt::intern(get_datatype_string()));
        global = pmt::dict_add(global, pmt::intern("core:version"), pmt::intern("1.0.0"));
        d_meta_dict = pmt::dict_add(d_meta_dict, pmt::intern("global"), global);
    }
}

/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl()
{
    // Write the metadata to a file and close both files
    d_meta_file << d_meta.dump(4) << std::endl;
    d_data_file.close();
    d_meta_file.close();
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
            d_meta_dict = pmt::dict_update(d_meta_dict,d_meta_queue.front());
            d_data_queue.pop();
            d_meta_queue.pop();
        }
        size_t n = pmt::length(d_data);
        d_data_file.write((char*)pmt::blob_data(d_data), n * d_itemsize);
        // If the user wants metadata and we have some, save it
        if (d_meta_file.is_open() and pmt::length(pmt::dict_keys(d_meta_dict)) > 0) {
            parse_meta(d_meta_dict, d_meta);
            // GR_LOG_DEBUG(d_logger, d_meta.dump(4));
            d_meta_dict = pmt::make_dict();
        }
    }
}

void pdu_file_sink_impl::parse_meta(const pmt::pmt_t& dict, nlohmann::json& json)
{
    pmt::pmt_t items = pmt::dict_items(dict);
    std::string key;
    pmt::pmt_t value;
    for (size_t i = 0; i < pmt::length(items); i++) {
        key = pmt::symbol_to_string(pmt::car(pmt::nth(i, items)));
        value = pmt::cdr(pmt::nth(i, items));

        if (pmt::is_number(value)) {
            if (pmt::is_uint64(value))
                json[key] = pmt::to_uint64(value);
            else if (pmt::is_integer(value))
                json[key] = pmt::to_long(value);
            else
                json[key] = pmt::to_double(value);
        } else if (pmt::is_dict(value)) {
            // Recursively add the dictionary to the json file
            nlohmann::json dict;
            parse_meta(value, dict);
            json[key] = dict;
        } else {
            json[key] = pmt::write_string(value);
        }
    }
}

std::string pdu_file_sink_impl::get_datatype_string()
{
    std::string outstr;
    switch (d_itemsize) {
    case sizeof(gr_complex):
        outstr = "cf";
        break;
    case sizeof(float):
        outstr = "f";
        break;
    case sizeof(short):
        outstr = "i";
        break;
    case sizeof(char):
        outstr += "u";
        break;
    default:
        break;
    }
    outstr += std::to_string(8 * d_itemsize);
    if (is_big_endian()) {
        outstr += "_be";
    } else {
        outstr += "_le";
    }


    return outstr;
}


} /* namespace plasma */
} /* namespace gr */

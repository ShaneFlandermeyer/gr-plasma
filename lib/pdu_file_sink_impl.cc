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
      itemsize(itemsize),
      data_filename(data_filename),
      meta_filename(meta_filename)
{

    message_port_register_in(PMT_IN);
    set_msg_handler(PMT_IN, [this](const pmt::pmt_t& msg) { handle_message(msg); });
    data_file = std::ofstream(data_filename, std::ios::binary | std::ios::out);
    if (not meta_filename.empty()) {
        meta_file = std::ofstream(meta_filename, std::ios::out);
    }
    meta = pmt::make_dict();
}

/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl()
{
    // Write the metadata to a file and close both files
    if (not meta_filename.empty()) {
        meta_file << meta_json.dump(2) << std::endl;
        meta_file.close();
    }
    data_file.close();
}

void pdu_file_sink_impl::handle_message(const pmt::pmt_t& msg)
{
    if (pmt::is_pdu(msg)) {
        gr::thread::scoped_lock lock(queue_mutex);
        data_queue.push(pmt::cdr(msg));
        meta_queue.push(pmt::car(msg));
        write_cond.notify_one();
    }
}

bool pdu_file_sink_impl::start()
{
    finished = false;
    main_thread = gr::thread::thread([this] { run(); });

    return block::start();
}

bool pdu_file_sink_impl::stop()
{
    finished = true;
    write_cond.notify_one();
    main_thread.join();

    return block::stop();
}

void pdu_file_sink_impl::run()
{
    static bool first = true;
    while (true) {
        {
            gr::thread::scoped_lock lock(queue_mutex);
            write_cond.wait(lock, [this] { return not data_queue.empty() || finished; });
            if (finished)
                return;
            data = data_queue.front();
            meta = pmt::dict_update(meta, meta_queue.front());
            data_queue.pop();
            meta_queue.pop();
        }
        size_t n = pmt::length(data);
        data_file.write((char*)pmt::blob_data(data), n * itemsize);
        // If the user wants metadata and we have some, save it
        if (meta_file.is_open() and pmt::length(pmt::dict_keys(meta)) > 0) {
            parse_meta(meta, meta_json);
            meta = pmt::make_dict();
        }

        if (first) {
            std::string datatype = get_datatype_string();
            meta_json["datatype"] = datatype;
            first = false;
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
            json[key].emplace_back(dict);
        } else {
            json[key] = pmt::write_string(value);
        }
    }
}

std::string pdu_file_sink_impl::get_datatype_string()
{
    std::string outstr;
    switch (itemsize) {
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
    outstr += std::to_string(8 * itemsize);
    if (is_big_endian()) {
        outstr += "_be";
    } else {
        outstr += "_le";
    }


    return outstr;
}


} /* namespace plasma */
} /* namespace gr */

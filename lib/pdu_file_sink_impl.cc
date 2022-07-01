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
    message_port_register_in(pmt::mp("in"));
    set_msg_handler(pmt::mp("in"),
                    [this](const pmt::pmt_t& msg) { handle_message(msg); });
    d_data_file = std::ofstream(d_data_filename, std::ios::binary | std::ios::out);
    if (not d_meta_filename.empty()) {
        d_meta_file = std::ofstream(d_meta_filename, std::ios::out);
        // Initialize the global metadata fields
        d_global["core:datatype"] = get_datatype_string();
        d_global["core:version"] = "1.0.0";
        d_meta["global"] = d_global;
    }
}

/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl()
{

    if (not d_annotation.empty()) {
        d_meta["annotations"].emplace_back(d_annotation);
    }
    if (not d_capture.empty()) {
        d_meta["captures"].emplace_back(d_capture);
    }
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
            d_meta_dict = d_meta_queue.front();
            d_data_queue.pop();
            d_meta_queue.pop();
        }
        size_t n = pmt::length(d_data);
        d_data_file.write((char*)pmt::blob_data(d_data), n * d_itemsize);
        // If the user wants metadata and we have some, save it
        if (d_meta_file.is_open() and pmt::length(pmt::dict_keys(d_meta_dict)) > 0) {
            parse_meta(d_meta_dict);
        }
    }
}

void pdu_file_sink_impl::parse_meta(const pmt::pmt_t& dict)
{
    // Global object fields
    pmt::pmt_t sample_rate = pmt::dict_ref(dict, SAMPLE_RATE_KEY, pmt::PMT_NIL);
    // Capture object fields
    pmt::pmt_t frequency = pmt::dict_ref(dict, FREQUENCY_KEY, pmt::PMT_NIL);
    // Annotation object fields
    pmt::pmt_t sample_start = pmt::dict_ref(dict, SAMPLE_START_KEY, pmt::PMT_NIL);
    pmt::pmt_t sample_count =
        pmt::dict_ref(dict,SAMPLE_COUNT_KEY , pmt::PMT_NIL);
    pmt::pmt_t bandwidth = pmt::dict_ref(dict, BANDWIDTH_KEY, pmt::PMT_NIL);
    pmt::pmt_t label = pmt::dict_ref(dict, LABEL_KEY, pmt::PMT_NIL);


    // Update global object
    if (not pmt::is_null(sample_rate))
        d_capture["core:sample_rate"] = pmt::to_double(sample_rate);

    // Update capture object
    if (not pmt::is_null(frequency)) {
        if (d_annotation.contains("core:sample_start")) {
            // If a new sample start is given on a frequency change, start a new
            // capture segment
            d_meta["captures"].emplace_back(d_capture);
            d_capture.clear();
        }
        d_capture["core:frequency"] = pmt::to_double(frequency);
    }


    // Update annotation object
    if (not pmt::is_null(sample_start)) {
        if (d_annotation.contains("core:sample_start")) {
            // Create a new annotation segment each time a sample start key is sent
            d_meta["annotations"].emplace_back(d_annotation);
            d_annotation.clear();
        }
        d_annotation["core:sample_start"] = pmt::to_uint64(sample_start);
    }

    if (not pmt::is_null(sample_count))
        d_annotation["core:sample_count"] = pmt::to_uint64(sample_count);
    if (not pmt::is_null(label))
        d_annotation["core:label"] = pmt::symbol_to_string(label);
    if (not pmt::is_null(bandwidth)) {
        double lower = -pmt::to_double(bandwidth) / 2;
        double upper = pmt::to_double(bandwidth) / 2;
        if (not pmt::is_null(frequency)) {
            lower += pmt::to_double(frequency);
            upper += pmt::to_double(frequency);
        }
        d_annotation["core:freq_lower_edge"] = lower;
        d_annotation["core:freq_upper_edge"] = upper;
    }
    // d_meta["captures"].emplace_back(d_capture);
    // d_meta["annotations"].emplace_back(d_annotation);

    // d_capture.clear();
    // d_annotation.clear();
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

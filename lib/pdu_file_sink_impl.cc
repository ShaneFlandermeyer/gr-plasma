/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_file_sink_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

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
        // Initialize the global metadata fields
        d_sigmf_meta.global.access<core::GlobalT>().datatype = get_datatype_string();
    }
}

/*
 * Our virtual destructor.
 */
pdu_file_sink_impl::~pdu_file_sink_impl()
{
    // Write the metadata to a file
    // d_sigmf_meta.annotations.emplace_back(anno);
    d_meta_file << json(d_sigmf_meta).dump(4) << std::endl;
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
        d_data_file.write((char*)pmt::c32vector_writable_elements(d_data, n),
                          n * sizeof(gr_complex));
        // If the user wants metadata and we have some, save it
        if (d_meta_file.is_open() and pmt::length(pmt::dict_keys(d_meta_dict)) > 0) {
            parse_meta(d_meta_dict);
        }
    }
}

void pdu_file_sink_impl::parse_meta(const pmt::pmt_t& dict)
{
    // sigmf::Capture<core::DescrT> cap;
    // sigmf::Annotation<core::DescrT, signal::DescrT>
    sigmf::Capture<core::DescrT> cap;
    sigmf::Annotation<core::DescrT, signal::DescrT> anno;

    // Global object fields
    pmt::pmt_t sample_rate =
        pmt::dict_ref(dict, pmt::intern("sample_rate"), pmt::PMT_NIL);
    // Capture object fields
    pmt::pmt_t frequency = pmt::dict_ref(dict, pmt::intern("frequency"), pmt::PMT_NIL);
    // Annotation object fields
    pmt::pmt_t sample_start =
        pmt::dict_ref(dict, pmt::intern("sample_start"), pmt::PMT_NIL);
    pmt::pmt_t sample_count =
        pmt::dict_ref(dict, pmt::intern("sample_count"), pmt::PMT_NIL);
    pmt::pmt_t bandwidth = pmt::dict_ref(dict, pmt::intern("bandwidth"), pmt::PMT_NIL);
    pmt::pmt_t label = pmt::dict_ref(dict, pmt::intern("label"), pmt::PMT_NIL);


    // Update global object
    if (not pmt::is_null(sample_rate))
        d_sigmf_meta.global.access<core::GlobalT>().sample_rate =
            pmt::to_double(sample_rate);

    // Update capture object
    if (not pmt::is_null(frequency)) {
        cap.get<core::DescrT>().frequency = pmt::to_double(frequency);
        if (not pmt::is_null(sample_start)) 
            cap.get<core::DescrT>().sample_start = pmt::to_uint64(sample_start);
    }


    // Update annotation object
    if (not pmt::is_null(sample_start))
        anno.get<core::DescrT>().sample_start = pmt::to_uint64(sample_start);
    if (not pmt::is_null(sample_count))
        anno.get<core::DescrT>().sample_count = pmt::to_uint64(sample_count);
    if (not pmt::is_null(label))
        anno.get<core::DescrT>().label = pmt::symbol_to_string(label);
    if (not pmt::is_null(bandwidth)) {
        double bw = pmt::to_double(bandwidth);
        if (pmt::is_null(frequency)) {
            // Center frequency not specified. Specify the bandwidth at complex
            // baseband
            anno.get<core::DescrT>().freq_lower_edge = -bw / 2;
            anno.get<core::DescrT>().freq_upper_edge = bw / 2;
        } else {
            // Center frequency specified. Save the frequency at RF
            double center_freq = pmt::to_double(frequency);
            anno.get<core::DescrT>().freq_lower_edge = -bw / 2 + center_freq;
            anno.get<core::DescrT>().freq_upper_edge = bw / 2 + center_freq;
        }
    }
    d_sigmf_meta.captures.emplace_back(cap);
    d_sigmf_meta.annotations.emplace_back(anno);
}

std::string pdu_file_sink_impl::get_datatype_string()
{
#pragma message("TODO: Don't hardcode the datatype string")
    return "cf32_le";
}


} /* namespace plasma */
} /* namespace gr */

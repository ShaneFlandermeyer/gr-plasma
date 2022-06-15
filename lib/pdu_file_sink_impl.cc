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
    d_sigmf_meta.annotations.emplace_back(d_annotation);
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
#pragma message("TODO: Save actual metadata in the PDU file sink")
        if (d_meta_file.is_open()) {
            // Update the d_capture segment

            // Update sample rate
            if (pmt::dict_has_key(d_meta_dict, pmt::intern("sample_rate"))) {
                d_sigmf_meta.global.access<core::GlobalT>().sample_rate = pmt::to_double(
                    pmt::dict_ref(d_meta_dict, pmt::intern("sample_rate"), pmt::PMT_NIL));
            }
            // Update center frequency
            if (pmt::dict_has_key(d_meta_dict, pmt::intern("frequency"))) {
                d_capture.get<core::DescrT>().frequency = pmt::to_double(
                    pmt::dict_ref(d_meta_dict, pmt::intern("frequency"), pmt::PMT_NIL));
                // TODO: Don't hard-code this
                d_capture.get<core::DescrT>().sample_start = 0;
                d_sigmf_meta.captures.emplace_back(d_capture);
            }
            // Update bandwidth
            if (pmt::dict_has_key(d_meta_dict, pmt::intern("bandwidth"))) {
                double bandwidth = pmt::to_double(
                    pmt::dict_ref(d_meta_dict, pmt::intern("bandwidth"), pmt::PMT_NIL));
                try {
                    d_annotation.get<core::DescrT>().freq_lower_edge =
                        -bandwidth / 2 + d_capture.get<core::DescrT>().frequency.value();
                    d_annotation.get<core::DescrT>().freq_upper_edge =
                        bandwidth / 2 + d_capture.get<core::DescrT>().frequency.value();
                } catch (...) {
                    d_annotation.get<core::DescrT>().freq_lower_edge =
                        -bandwidth / 2;
                    d_annotation.get<core::DescrT>().freq_upper_edge =
                        bandwidth / 2;
                }
            }

            // pmt::pmt_t sample_start =
            //     pmt::dict_ref(d_meta_dict, pmt::intern("sample_start"), pmt::PMT_NIL);
            // pmt::pmt_t bandwidth =
            //     pmt::dict_ref(d_meta_dict, pmt::intern("bandwidth"), pmt::PMT_NIL);
            // pmt::pmt_t label =
            //     pmt::dict_ref(d_meta_dict, pmt::intern("label"), pmt::PMT_NIL);

            // if (not pmt::is_null(bandwidth)) {
            //     // If a center frequency is given, use it to compute the
            //     // frequency edges. Otherwise, specify the edges at complex baseband.
            //     if (not pmt::is_null(frequency)) {
            //         double freq = pmt::to_double(frequency);
            //         d_capture.get<core::DescrT>().frequency = freq;
            //         d_annotation.get<core::DescrT>().freq_lower_edge =
            //             -pmt::to_double(bandwidth) / 2 + freq;
            //         d_annotation.get<core::DescrT>().freq_upper_edge =
            //             pmt::to_double(bandwidth) / 2 + freq;
            //     } else {
            //         d_annotation.get<core::DescrT>().freq_lower_edge =
            //             -pmt::to_double(bandwidth) / 2;
            //         d_annotation.get<core::DescrT>().freq_upper_edge =
            //             pmt::to_double(bandwidth) / 2;
            //     }
            // }

            // if (not pmt::is_null(label))
            //     d_annotation.get<core::DescrT>().label = pmt::symbol_to_string(label);

            // if (not pmt::is_null(sample_start)) {
            //     size_t start = pmt::to_long(sample_start);
            //     if (start > 0 and
            //         start != d_annotation.get<core::DescrT>().sample_start.value()) {
            //         d_annotation.get<core::DescrT>().sample_count =
            //             start - d_annotation.get<core::DescrT>().sample_start.value();
            //         d_sigmf_meta.annotations.emplace_back(d_annotation);
            //     }
            //     d_annotation.get<core::DescrT>().sample_start = start;
            // }


            // Create a new annotation when the waveform changes


            // Create a new d_capture segment when the center frequency changes
        }
    }
}

std::string pdu_file_sink_impl::get_datatype_string()
{
#pragma message("TODO: Don't hardcode the datatype string")
    return "cf32_le";
}

// void pdu_file_sink_impl::update_global_fields()
// {

// }


} /* namespace plasma */
} /* namespace gr */

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
            update_global_fields(d_sigmf_meta);
            // Add capture segment
            auto capture = sigmf::Capture<core::DescrT>();
            capture.get<core::DescrT>().sample_start = 0;
            pmt::pmt_t frequency =
                pmt::dict_ref(d_meta_dict, pmt::intern("frequency"), pmt::PMT_NIL);
            if (not pmt::eq(frequency, pmt::PMT_NIL))
                capture.get<core::DescrT>().frequency = pmt::to_double(frequency);
            d_sigmf_meta.captures.emplace_back(capture);


            // sigmf::SigMF<sigmf::Global<core::DescrT>,
            //              sigmf::Capture<core::DescrT>,
            //              sigmf::Annotation<core::DescrT>>
            //     latest_record;
            //
            // latest_record.global.access<core::GlobalT>().author = "Nathan";
            // latest_record.global.access<core::GlobalT>().description =
            //     "Example of creating a new record";
            // latest_record.global.access<core::GlobalT>().sample_rate = 1.0;
            // latest_record.global.access<antenna::GlobalT>().gain = 40.0;
            // latest_record.global.access<antenna::GlobalT>().low_frequency = 600e6;
            // latest_record.global.access<antenna::GlobalT>().high_frequency = 1200e6;

            // // Add a capture segment
            // auto antenna_capture = sigmf::Capture<core::DescrT>();
            // antenna_capture.get<core::DescrT>().frequency = 870e6;
            // antenna_capture.get<core::DescrT>().global_index = 0;
            // latest_record.captures.emplace_back(antenna_capture);

            // auto& fancy_capture = latest_record.captures.create_new();
            // auto& fancy_cap_core = fancy_capture.get<core::DescrT>();
            // fancy_cap_core.datetime = "the future";
            // fancy_cap_core.sample_start = 9001;


            // // Add some annotations (sigmf::core_annotations is typedef of
            // // core::AnnotationT, so they're interchangeable) This example uses the
            // // core::AnnotationT to access data elements which is more using the
            // // VariadicDataClass interface
            // auto anno2 = sigmf::Annotation<core::DescrT, antenna::DescrT>();
            // anno2.access<core::AnnotationT>().sample_count = 500000;
            // anno2.access<core::AnnotationT>().description = "Annotation 1";
            // anno2.access<core::AnnotationT>().generator = "libsigmf";
            // anno2.access<core::AnnotationT>().description = "Woah!";
            // latest_record.annotations.emplace_back(anno2);

            // // This example shows off using the Annotation-specific interface where we
            // // know it's an annotation, so we get annotation field from the underlying
            // // DescrT... This uses a little bit of syntactic sugar on top of the
            // // VariadicDataClass and basically you don't have to repeat "annotation" in
            // // your get/access method.
            // auto anno3 = sigmf::Annotation<core::DescrT, antenna::DescrT>();
            // anno3.get<core::DescrT>().sample_count = 600000;
            // anno3.get<core::DescrT>().sample_count = 600000;
            // anno3.get<core::DescrT>().description = "Annotation 2";
            // anno3.get<core::DescrT>().generator = "libsigmf";
            // anno3.get<core::DescrT>().description = "Pretty easy";
            // anno3.get<antenna::DescrT>().elevation_angle = 4.2;
            // // You can also drop in this syntactic acid using this interface which I
            // // personally don't really like because it mixes real calls with macros
            // // without it being obvious and doesn't really feel like c++
            // anno3.sigmfns(antenna).azimuth_angle = 0.1;
            // anno3.get<antenna::DescrT>().polarization = "circular";

            // latest_record.annotations.emplace_back(anno3);

            // for (size_t i = 0; i < pmt::length(items); i++) {
            //     // std::string key = pmt::write_string(pmt::car(pmt::nth(i, items)));
            //     // std::string value = pmt::write_string(pmt::cdr(pmt::nth(i, items)));
            //     // std::string line = key + ": " + value + "\n";
            //     d_meta_file.write(line.c_str(), line.size());
            // }
            // d_meta_file.write("\n", 1);
        }
    }
}

std::string pdu_file_sink_impl::get_datatype_string()
{
#pragma message("TODO: Don't hardcode the datatype string")
    return "cf32_le";
}

void pdu_file_sink_impl::update_global_fields(
    sigmf::SigMF<sigmf::Global<core::DescrT>,
                 sigmf::Capture<core::DescrT>,
                 sigmf::Annotation<core::DescrT>>& meta)
{
    meta.global.access<core::GlobalT>().sample_rate = pmt::to_double(
        pmt::dict_ref(d_meta_dict, pmt::intern("sample_rate"), pmt::PMT_NIL));
}


} /* namespace plasma */
} /* namespace gr */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_file_source_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

pdu_file_source::sptr pdu_file_source::make(const std::string& data_filename,
                                            const std::string& meta_filename,
                                            int offset,
                                            int length)
{
    return gnuradio::make_block_sptr<pdu_file_source_impl>(
        data_filename, meta_filename, offset, length);
}


/*
 * The private constructor
 */
pdu_file_source_impl::pdu_file_source_impl(const std::string& data_filename,
                                           const std::string& meta_filename,
                                           int offset,
                                           int length)
    : gr::block("pdu_file_source",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_data_filename(data_filename),
      d_meta_filename(meta_filename),
      d_offset(offset),
      d_length(length)
{
    // Read I/Q data from file
    std::vector<gr_complex> data =
        ::plasma::read<gr_complex>(d_data_filename, d_offset, d_length);
    d_data = pmt::init_c32vector(data.size(), data.data());

    // Load metadata
    d_meta = pmt::make_dict();
    if (not d_meta_filename.empty()) {
        std::ifstream meta_file(d_meta_filename);
        nlohmann::json json = nlohmann::json::parse(meta_file);
        d_meta = parse_meta(json);
    }

    d_out_port = PMT_OUT;
    message_port_register_out(d_out_port);
}

/*
 * Our virtual destructor.
 */
pdu_file_source_impl::~pdu_file_source_impl() {}

bool pdu_file_source_impl::start()
{
    d_thread = gr::thread::thread([this] { run(); });
    return block::start();
}

bool pdu_file_source_impl::stop()
{
    d_thread.join();
    return block::stop();
}

void pdu_file_source_impl::run()
{
    message_port_pub(pmt::mp("out"), pmt::cons(d_meta, d_data));
}

pmt::pmt_t pdu_file_source_impl::parse_meta(const nlohmann::json& json)
{
    pmt::pmt_t dict = pmt::make_dict();
    for (const auto& item : json.items()) {
        pmt::pmt_t pmt_key = pmt::intern(item.key());
        nlohmann::json value = item.value();
        if (value.is_structured()) {
            // Recursively parse the JSON object representing this value
            dict = pmt::dict_add(dict, pmt_key, parse_meta(value));
        } else {
            // Parse the primitive JSON values as dictionary key:value pairs
            pmt::pmt_t pmt_value;
            if (value.is_string())
                pmt_value = pmt::intern(value);
            else if (value.is_number_float())
                pmt_value = pmt::from_double(value);
            else if (value.is_number_integer())
                pmt_value = pmt::from_long(value);
            else if (value.is_number_unsigned())
                pmt_value = pmt::from_uint64(value);
            else if (value.is_boolean())
                pmt_value = pmt::from_bool(value);
            dict = pmt::dict_add(dict, pmt_key, pmt_value);
        }
    }
    return dict;
}

} /* namespace plasma */
} /* namespace gr */
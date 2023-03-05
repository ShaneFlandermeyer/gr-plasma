/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_FILE_SOURCE_IMPL_H
#define INCLUDED_PLASMA_PDU_FILE_SOURCE_IMPL_H

#include <gnuradio/plasma/pdu_file_source.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <nlohmann/json.hpp>
#include <plasma_dsp/file.h>
#include <fstream>

namespace gr {
namespace plasma {

class pdu_file_source_impl : public pdu_file_source
{
private:
    std::string d_data_filename;
    std::string d_meta_filename;
    int d_offset;
    int d_length;
    int d_pdu_length;
    gr::thread::thread d_thread;
    std::vector<gr_complex> d_data;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_out_port;

    /**
     * @brief Convert a JSON object to a PMT dictionary
     * 
     * Note that this function is recursive and will convert nested JSON objects
     * to dictionaries of dictionaries.
     * 
     * @param json 
     * @return pmt::pmt_t 
     */
    pmt::pmt_t parse_meta(const nlohmann::json& json);

public:
    pdu_file_source_impl(const std::string& data_filename,
                         const std::string& meta_filename,
                         int offset,
                         int length,
                         int pdu_length);
    ~pdu_file_source_impl();

    bool start() override;
    bool stop() override;
    void run();
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_FILE_SOURCE_IMPL_H */
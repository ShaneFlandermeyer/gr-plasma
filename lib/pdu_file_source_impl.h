/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_FILE_SOURCE_IMPL_H
#define INCLUDED_PLASMA_PDU_FILE_SOURCE_IMPL_H

#include <gnuradio/plasma/pdu_file_source.h>
#include <plasma_dsp/file.h>
#include <fstream>

namespace gr {
namespace plasma {

class pdu_file_source_impl : public pdu_file_source
{
private:
    std::string d_data_filename;
    std::string d_meta_filename;
    size_t d_offset;
    size_t d_length;
    std::atomic<bool> d_finished;
    gr::thread::thread d_thread;
    pmt::pmt_t d_data;
    pmt::pmt_t d_meta;
    

    // std::ifstream d_data_file;
    // std::ifstream d_meta_file;
    

public:
    pdu_file_source_impl(const std::string &data_filename,
                         const std::string &meta_filename,
                         size_t offset,
                         size_t length);
    ~pdu_file_source_impl();

    bool start() override;
    bool stop() override;
    void run();
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_FILE_SOURCE_IMPL_H */

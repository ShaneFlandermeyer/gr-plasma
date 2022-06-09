/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H
#define INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H

#include <gnuradio/plasma/pdu_file_sink.h>
#include <fstream>
#include <queue>


namespace gr {
namespace plasma {

class pdu_file_sink_impl : public pdu_file_sink
{
private:
    // Nothing to declare in this block.
    std::vector<gr_complex> d_data;
    std::ofstream d_file;
    std::string d_filename;
    size_t d_num_pulse_cpi;
    size_t d_num_samp_pulse;
    size_t d_current_pulse_index;

public:
    pdu_file_sink_impl(size_t num_pulse_cpi,const std::string& filename);
    ~pdu_file_sink_impl();

    void handle_message(const pmt::pmt_t& msg);

    // Where all the action really happens
    // int work(int noutput_items,
    //          gr_vector_const_void_star& input_items,
    //          gr_vector_void_star& output_items);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H */

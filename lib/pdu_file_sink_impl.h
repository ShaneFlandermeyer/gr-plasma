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
    size_t d_num_pulse_cpi;
    std::string d_filename;
    std::queue<const gr_complex*> d_pulse_queue;    
    std::ofstream d_file;
    size_t d_num_samp_pulse;
    gr::thread::thread d_thread;
    std::atomic<bool> d_finished;
    std::mutex d_mutex;
    std::unique_lock<std::mutex> d_lock;

public:
    pdu_file_sink_impl(size_t num_pulse_cpi,const std::string& filename);
    ~pdu_file_sink_impl();

    void handle_message(const pmt::pmt_t& msg);

    bool start() override;
    bool stop() override;
    void run();



    // Where all the action really happens
    // int work(int noutput_items,
    //          gr_vector_const_void_star& input_items,
    //          gr_vector_void_star& output_items);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H */

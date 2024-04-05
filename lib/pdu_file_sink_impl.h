/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H
#define INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H

#include <gnuradio/plasma/pdu_file_sink.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <nlohmann/json.hpp>
#include <uhd/utils/thread.hpp>
#include <fstream>
#include <queue>

namespace gr {
namespace plasma {

class pdu_file_sink_impl : public pdu_file_sink
{
private:
    size_t itemsize;
    std::queue<pmt::pmt_t> data_queue, meta_queue;
    pmt::pmt_t data, meta;
    nlohmann::json meta_json;

    std::string data_filename, meta_filename;
    std::ofstream data_file, meta_file;

    // Threading
    gr::thread::thread main_thread;
    gr::thread::mutex queue_mutex;
    gr::thread::condition_variable write_cond;
    std::atomic<bool> finished;


    std::string get_datatype_string();
    void parse_meta(const pmt::pmt_t& dict, nlohmann::json& json);


public:
    pdu_file_sink_impl(size_t itemsize,
                       std::string& data_filename,
                       std::string& meta_filename);
    ~pdu_file_sink_impl();

    /**
     * @brief Process an input message item
     *
     * @param msg The input message to be processed. If this is a PDU, its data
     * and metadata is added to the queue.
     *
     * TODO: Handle control messages as well
     */
    void handle_message(const pmt::pmt_t& msg);

    bool start() override;
    bool stop() override;

    /**
     * @brief Start the worker thread to concurrently write data to a file
     *
     */
    void run();
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H */

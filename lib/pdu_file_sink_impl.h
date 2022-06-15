/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H
#define INCLUDED_PLASMA_PDU_FILE_SINK_IMPL_H

#include <gnuradio/plasma/pdu_file_sink.h>
#include <uhd/utils/thread.hpp>
#include <fstream>
#include <queue>

#include <sigmf/sigmf.h>
#include <sigmf/sigmf_antenna_generated.h>
#include <sigmf/sigmf_core_generated.h>

namespace gr {
namespace plasma {

class pdu_file_sink_impl : public pdu_file_sink
{
private:
    /**
     * @brief Name of the data file
     *
     */
    std::string d_data_filename;

    /**
     * @brief Name of the metadata file
     *
     */
    std::string d_meta_filename;

    /**
     * @brief Queue of data to be written to a file
     *
     */
    std::queue<pmt::pmt_t> d_data_queue;

    /**
     * @brief Queue of metadata to be written to a file
     *
     */
    std::queue<pmt::pmt_t> d_meta_queue;

    /**
     * @brief File stream to write data to
     *
     */
    std::ofstream d_data_file;

    /**
     * @brief File stream to write metadata to
     *
     */
    std::ofstream d_meta_file;

    /**
     * @brief Worker thread used for run() method
     *
     */
    gr::thread::thread d_thread;

    /**
     * @brief Mutex to be acquired when accessing the queues
     *
     */
    gr::thread::mutex d_mutex;

    /**
     * @brief Condition variable that signals the worker thread to write to file
     *
     */
    gr::thread::condition_variable d_cond;

    /**
     * @brief Indicates that the block should clean up and shut down
     *
     */
    std::atomic<bool> d_finished;

    /**
     * @brief Current data item to be written to file
     *
     */
    pmt::pmt_t d_data;

    /**
     * @brief Current metadata item to be written to file
     *
     */
    pmt::pmt_t d_meta_dict;

    sigmf::SigMF<sigmf::Global<core::DescrT>,
                 sigmf::Capture<core::DescrT>,
                 sigmf::Annotation<core::DescrT>>
        d_sigmf_meta;

    std::string get_datatype_string();

    void update_global_fields(sigmf::SigMF<sigmf::Global<core::DescrT>,
                                           sigmf::Capture<core::DescrT>,
                                           sigmf::Annotation<core::DescrT>>& meta);


public:
    pdu_file_sink_impl(std::string& data_filename, std::string& meta_filename);
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

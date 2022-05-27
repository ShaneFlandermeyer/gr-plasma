/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lfm_source_impl.h"
#include <gnuradio/io_signature.h>
#include <chrono>

namespace gr {
namespace plasma {

using output_type = gr_complex;
lfm_source::sptr
lfm_source::make(double bandwidth, double pulse_width, double prf, double samp_rate)
{
    return gnuradio::make_block_sptr<lfm_source_impl>(
        bandwidth, pulse_width, prf, samp_rate);
}


/*
 * The private constructor
 */
lfm_source_impl::lfm_source_impl(double bandwidth,
                                 double pulse_width,
                                 double prf,
                                 double samp_rate)
    : gr::sync_block("lfm_source",
                     gr::io_signature::make(0, 0, 0),
                     gr::io_signature::make(
                         1 /* min outputs */, 1 /*max outputs */, sizeof(output_type))),
      d_port(pmt::mp("pdu")),
      d_prf(prf),
      d_sample_index(0)

{
    using namespace std::chrono;
    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    d_epoch = epoch;
    d_start_time =
        duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    d_send_time = d_start_time + 1e5;
    d_waveform = ::plasma::LinearFMWaveform(bandwidth, pulse_width, prf, samp_rate);
    d_data = d_waveform.step().cast<gr_complex>();
    message_port_register_out(d_port);
}

/*
 * Our virtual destructor.
 */
lfm_source_impl::~lfm_source_impl() {}

/**
 * @brief Start the main work thread and the pdu thread
 * 
 * @return true 
 * @return false 
 */
bool lfm_source_impl::start()
{
    d_finished = false;
    d_thread = gr::thread::thread([this] { run(); });

    return block::start();
}

/**
 * @brief Stop both threads
 * 
 * @return true 
 * @return false 
 */
bool lfm_source_impl::stop()
{
    d_finished = true;
    d_thread.interrupt();
    d_thread.join();

    return block::stop();
}

/**
 * @brief Sends the waveform PDU once per PRI. 
 * 
 * This PDU includes:
 *  - The data itself
 *  - The transmit time in the format required for the USRP blocks, where tx_time
 *    is a tuple where the first item is uint64 epoch time and the second item is a
 *    double fractional epoch time
 *  - The pulse length in samples
 * 
 * This metadata format allows the PDU to be passed through a PDU to tagged
 * stream block, which enables bursty transmission through the USRP sink.
 * 
 * 
 */
void lfm_source_impl::run()
{
    using namespace std::chrono;
    while (not d_finished) {
        while (duration_cast<microseconds>(system_clock::now().time_since_epoch()).count() < d_send_time - 1e3) {
            boost::this_thread::sleep(boost::posix_time::microseconds(1));
        }
        d_send_time += 1e6 / d_prf;
        if (d_finished)
            return;
        // Create the metadata dictionary and publish the PDU
        pmt::pmt_t meta = pmt::make_dict();
        meta =
            pmt::dict_add(meta,
                          pmt::mp("tx_time"),
                          pmt::make_tuple(pmt::from_uint64(d_send_time / 1e6),
                          pmt::from_double((d_send_time % (int)1e6) * 1e-6)));
        pmt::pmt_t data = pmt::init_c32vector(d_data.size(), d_data.data());
        message_port_pub(d_port, pmt::cons(meta, data));
    }
}

int lfm_source_impl::work(int noutput_items,
                          gr_vector_const_void_star& input_items,
                          gr_vector_void_star& output_items)
{
    // auto out = static_cast<output_type*>(output_items[0]);
    // if (d_data.size() < noutput_items) {
    //     // Normal for loop through data.size()
    // } else {
    //     // Loop through noutput_items, subtract off from data.size()
    // }
    // for (int i = 0; i < noutput_items; i++) {
    //     if (d_sample_index == 0) {
    //         add_item_tag(0, nitems_written(0) + i, pmt::mp("packet_len"),
    //                      pmt::from_long(d_data.size()), d_port);
    //         d_send_time += 1e6 / d_prf;
    //         add_item_tag(0, nitems_written(0) + i, pmt::mp("tx_time"),
    //                      pmt::make_tuple(pmt::from_uint64(d_send_time / 1e6),
    //                                      pmt::from_double((d_send_time % (int)1e6) * 1e-6)),
    //                      d_port);
    //     }
    //     out[i] = d_data[d_sample_index];
    //     d_sample_index = (d_sample_index + 1) % d_data.size();
    // }

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace plasma */
} /* namespace gr */

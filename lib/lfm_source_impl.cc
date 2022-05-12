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
    d_send_time = d_start_time + 1e3;
    d_waveform = ::plasma::LinearFMWaveform(bandwidth, pulse_width, prf, samp_rate);
    d_data = d_waveform.sample().cast<gr_complex>();

    message_port_register_out(d_port);
}

/*
 * Our virtual destructor.
 */
lfm_source_impl::~lfm_source_impl() {}

bool lfm_source_impl::start()
{
    d_finished = false;
    d_thread = gr::thread::thread([this] { run(); });

    return block::start();
}

bool lfm_source_impl::stop()
{
    d_finished = true;
    d_thread.interrupt();
    d_thread.join();

    return block::stop();
}

void lfm_source_impl::run()
{
    using namespace std::chrono;
    while (not d_finished) {
        boost::this_thread::sleep(
            boost::posix_time::milliseconds(static_cast<long>(1000.0 / d_prf)));
        d_send_time += 1e6 / d_prf;
        if (d_finished)
            return;
        pmt::pmt_t meta = pmt::make_dict();
        meta =
            pmt::dict_add(meta,
                          pmt::mp("tx_time"),
                          pmt::cons(pmt::from_uint64(d_send_time / 1e6),
                                    pmt::from_double((d_send_time % (int)1e6) * 1e-6)));
        meta = pmt::dict_add(meta, pmt::mp("tx_sob"), pmt::from_bool(true));
        meta = pmt::dict_add(meta, pmt::mp("tx_eob"), pmt::from_bool(true));
        pmt::pmt_t data = pmt::init_c32vector(d_data.size(), d_data.data());

        message_port_pub(d_port, pmt::cons(meta,data));
    }
}

int lfm_source_impl::work(int noutput_items,
                          gr_vector_const_void_star& input_items,
                          gr_vector_void_star& output_items)
{
    auto out = static_cast<output_type*>(output_items[0]);

    // if (d_data.size() < noutput_items) {
    //     // Normal for loop through data.size()
    // } else {
    //     // Loop through noutput_items, subtract off from data.size()
    // }
    for (int i = 0; i < noutput_items; i++) {
        out[i] = d_data[d_sample_index];
        d_sample_index = (d_sample_index + 1) % d_data.size();
    }

    // Tell runtime system how many output items we produced.
    return noutput_items;
}

} /* namespace plasma */
} /* namespace gr */

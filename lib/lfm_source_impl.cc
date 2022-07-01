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
lfm_source::sptr lfm_source::make(double bandwidth, double pulse_width, double samp_rate)
{
    return gnuradio::make_block_sptr<lfm_source_impl>(bandwidth, pulse_width, samp_rate);
}


/*
 * The private constructor
 */
lfm_source_impl::lfm_source_impl(double bandwidth, double pulse_width, double samp_rate)
    : gr::block(
          "lfm_source", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_port(pmt::mp("pdu"))
{
    d_waveform = ::plasma::LinearFMWaveform(bandwidth, pulse_width, 0, samp_rate);
    d_data = d_waveform.sample().cast<gr_complex>();
    message_port_register_out(d_port);
}

/*
 * Our virtual destructor.
 */
lfm_source_impl::~lfm_source_impl() {}

/**
 * @brief Send a PDU containing the waveform data and metadata
 *
 * @return true
 * @return false
 */
bool lfm_source_impl::start()
{
    d_finished = false;
    // Send a PDU containing the waveform and its metadata
    pmt::pmt_t meta = pmt::make_dict();
    meta = pmt::dict_add(meta, BANDWIDTH_KEY, pmt::from_double(d_waveform.bandwidth()));
    meta =
        pmt::dict_add(meta, PULSEWIDTH_KEY, pmt::from_double(d_waveform.pulse_width()));
    meta = pmt::dict_add(meta, SAMPLE_RATE_KEY, pmt::from_double(d_waveform.samp_rate()));
    meta = pmt::dict_add(meta, LABEL_KEY, pmt::intern("lfm"));
    pmt::pmt_t data = pmt::init_c32vector(d_data.size(), d_data.data());
    message_port_pub(d_port, pmt::cons(meta, data));


    return block::start();
}


} /* namespace plasma */
} /* namespace gr */

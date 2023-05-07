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
lfm_source::sptr lfm_source::make(
    double bandwidth, double start_freq, double pulse_width, double samp_rate, double prf)
{
    return gnuradio::make_block_sptr<lfm_source_impl>(
        bandwidth, start_freq, pulse_width, samp_rate, prf);
}


/*
 * The private constructor
 */
lfm_source_impl::lfm_source_impl(
    double bandwidth, double start_freq, double pulse_width, double samp_rate, double prf)
    : gr::block(
          "lfm_source", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_msg_port(PMT_IN),
      d_out_port(PMT_OUT),
      d_bandwidth(bandwidth),
      d_start_freq(start_freq),
      d_pulse_width(pulse_width),
      d_samp_rate(samp_rate),
      d_prf(prf)
{
    af::array waveform =
        ::plasma::lfm(start_freq, bandwidth, pulse_width, samp_rate, prf).as(c32);
    d_data = pmt::init_c32vector(
        waveform.elements(), reinterpret_cast<gr_complex*>(waveform.host<af::cfloat>()));

    message_port_register_in(d_msg_port);
    message_port_register_out(d_out_port);

    set_msg_handler(d_msg_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
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
    // Send a PDU containing the waveform and its metadata
    message_port_pub(d_out_port, pmt::cons(d_meta, d_data));

    return block::start();
}

void lfm_source_impl::handle_msg(pmt::pmt_t msg)
{
    // TODO: Handle dictionary inputs for changing many parameters at once
    pmt::pmt_t key = pmt::car(msg);
    pmt::pmt_t value = pmt::cdr(msg);

    if (pmt::equal(key, d_bandwidth_key)) {
        d_bandwidth = pmt::to_double(value);
        d_meta = pmt::dict_add(d_meta, d_bandwidth_key, pmt::from_double(d_bandwidth));
    } else if (pmt::equal(key, d_start_freq_key)) {
        d_start_freq = pmt::to_double(value);
        d_meta = pmt::dict_add(d_meta, d_start_freq_key, pmt::from_double(d_start_freq));
    } else if (pmt::equal(key, d_duration_key)) {
        d_pulse_width = pmt::to_double(value);
        d_meta = pmt::dict_add(d_meta, d_duration_key, pmt::from_double(d_pulse_width));
    } else if (pmt::equal(key, d_sample_rate_key)) {
        d_samp_rate = pmt::to_double(value);
        d_meta = pmt::dict_add(d_meta, d_sample_rate_key, pmt::from_double(d_samp_rate));
    } else if (pmt::equal(key, d_prf_key)) {
        d_prf = pmt::to_double(value);
        d_meta = pmt::dict_add(d_meta, d_prf_key, pmt::from_double(d_prf));
    }

    // Create the new waveform vector and emit it as a PDU
    af::array waveform =
        ::plasma::lfm(d_start_freq, d_bandwidth, d_pulse_width, d_samp_rate, d_prf).as(c32);
    d_data = pmt::init_c32vector(
        waveform.elements(), reinterpret_cast<gr_complex*>(waveform.host<af::cfloat>()));
    message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
}

void lfm_source_impl::init_meta_dict(const std::string& bandwidth_key,
                                     const std::string& start_freq_key,
                                     const std::string& duration_key,
                                     const std::string& sample_rate_key,
                                     const std::string& label_key,
                                     const std::string& prf_key)
{
    d_bandwidth_key = pmt::intern(bandwidth_key);
    d_start_freq_key = pmt::intern(start_freq_key);
    d_duration_key = pmt::intern(duration_key);
    d_sample_rate_key = pmt::intern(sample_rate_key);
    d_label_key = pmt::intern(label_key);
    d_prf_key = pmt::intern(prf_key);


    d_meta = pmt::make_dict();
    d_meta = pmt::dict_add(d_meta, d_bandwidth_key, pmt::from_double(d_bandwidth));
    d_meta = pmt::dict_add(d_meta, d_start_freq_key, pmt::from_double(d_start_freq));
    d_meta = pmt::dict_add(d_meta, d_duration_key, pmt::from_double(d_pulse_width));
    d_meta = pmt::dict_add(d_meta, d_sample_rate_key, pmt::from_double(d_samp_rate));
    d_meta = pmt::dict_add(d_meta, d_label_key, pmt::intern("lfm"));
    d_meta = pmt::dict_add(d_meta, d_prf_key, pmt::from_double(d_prf));
}

} /* namespace plasma */
} /* namespace gr */

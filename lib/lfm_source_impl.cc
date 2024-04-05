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
      msg_port(PMT_IN),
      out_port(PMT_OUT),
      bandwidth(bandwidth),
      start_freq(start_freq),
      pulse_width(pulse_width),
      samp_rate(samp_rate),
      prf(prf)
{
    af::array samples =
        ::plasma::lfm(start_freq, bandwidth, pulse_width, samp_rate, prf).as(c32);
    pmt_samples = pmt::init_c32vector(
        samples.elements(), reinterpret_cast<gr_complex*>(samples.host<af::cfloat>()));

    message_port_register_in(msg_port);
    message_port_register_out(out_port);

    set_msg_handler(msg_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
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
    message_port_pub(out_port, pmt::cons(meta, pmt_samples));

    return block::start();
}

void lfm_source_impl::handle_msg(pmt::pmt_t msg)
{
    // TODO: Handle dictionary inputs for changing many parameters at once
    pmt::pmt_t key = pmt::car(msg);
    pmt::pmt_t value = pmt::cdr(msg);

    if (pmt::equal(key, bandwidth_key)) {
        bandwidth = pmt::to_double(value);
        meta = pmt::dict_add(meta, bandwidth_key, pmt::from_double(bandwidth));
    } else if (pmt::equal(key, start_freq_key)) {
        start_freq = pmt::to_double(value);
        meta = pmt::dict_add(meta, start_freq_key, pmt::from_double(start_freq));
    } else if (pmt::equal(key, duration_key)) {
        pulse_width = pmt::to_double(value);
        meta = pmt::dict_add(meta, duration_key, pmt::from_double(pulse_width));
    } else if (pmt::equal(key, sample_rate_key)) {
        samp_rate = pmt::to_double(value);
        meta = pmt::dict_add(meta, sample_rate_key, pmt::from_double(samp_rate));
    } else if (pmt::equal(key, prf_key)) {
        prf = pmt::to_double(value);
        meta = pmt::dict_add(meta, prf_key, pmt::from_double(prf));
    }

    // Create the new waveform vector and emit it as a PDU
    af::array waveform =
        ::plasma::lfm(start_freq, bandwidth, pulse_width, samp_rate).as(c32);
    pmt_samples = pmt::init_c32vector(
        waveform.elements(), reinterpret_cast<gr_complex*>(waveform.host<af::cfloat>()));
    message_port_pub(out_port, pmt::cons(meta, pmt_samples));
}

void lfm_source_impl::init_meta_dict(const std::string& bandwidth_key,
                                     const std::string& start_freq_key,
                                     const std::string& duration_key,
                                     const std::string& sample_rate_key,
                                     const std::string& label_key,
                                     const std::string& prf_key)
{
    this->bandwidth_key = pmt::intern(bandwidth_key);
    this->start_freq_key = pmt::intern(start_freq_key);
    this->duration_key = pmt::intern(duration_key);
    this->sample_rate_key = pmt::intern(sample_rate_key);
    this->label_key = pmt::intern(label_key);
    this->prf_key = pmt::intern(prf_key);


    meta = pmt::make_dict();
    meta = pmt::dict_add(meta, this->bandwidth_key, pmt::from_double(bandwidth));
    meta = pmt::dict_add(meta, this->start_freq_key, pmt::from_double(start_freq));
    meta = pmt::dict_add(meta, this->duration_key, pmt::from_double(pulse_width));
    meta = pmt::dict_add(meta, this->sample_rate_key, pmt::from_double(samp_rate));
    meta = pmt::dict_add(meta, this->label_key, pmt::intern("lfm"));
    meta = pmt::dict_add(meta, this->prf_key, pmt::from_double(prf));
}

} /* namespace plasma */
} /* namespace gr */

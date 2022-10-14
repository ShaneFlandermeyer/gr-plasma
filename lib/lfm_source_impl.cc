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
      d_port(PMT_OUT),
      d_bandwidth(bandwidth),
      d_pulse_width(pulse_width),
      d_samp_rate(samp_rate)
{
    d_waveform = ::plasma::LinearFMWaveform(bandwidth, pulse_width, samp_rate);
    // Generate the waveform data
    af::array waveform_array = d_waveform.sample().as(c32);
    d_num_samp = waveform_array.elements();
    d_data.reset(reinterpret_cast<gr_complex*>(waveform_array.host<af::cfloat>()));

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
    // Send a PDU containing the waveform and its metadata
    pmt::pmt_t data = pmt::init_c32vector(d_num_samp, d_data.get());
    message_port_pub(d_port, pmt::cons(d_meta, data));

    return block::start();
}

void lfm_source_impl::set_metadata_keys(std::string label_key,
                                        std::string sample_rate_key,
                                        std::string bandwidth_key,
                                        std::string duration_key)
{
    d_label_key = pmt::intern(label_key);
    d_sample_rate_key = pmt::intern(sample_rate_key);
    d_bandwidth_key = pmt::intern(bandwidth_key);
    d_duration_key = pmt::intern(duration_key);

    d_meta = pmt::make_dict();
    d_meta = pmt::dict_add(d_meta, d_label_key, pmt::intern("lfm"));
    d_meta = pmt::dict_add(d_meta, d_sample_rate_key, pmt::from_double(d_samp_rate));
    d_meta = pmt::dict_add(d_meta, d_bandwidth_key, pmt::from_double(d_bandwidth));
    d_meta = pmt::dict_add(d_meta, d_duration_key, pmt::from_double(d_pulse_width));
}

} /* namespace plasma */
} /* namespace gr */

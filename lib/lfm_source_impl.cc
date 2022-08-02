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
      d_port(PMT_PDU)
{
    d_waveform = ::plasma::LinearFMWaveform(bandwidth, pulse_width, 0, samp_rate);
    // Generate the waveform data
    af::array waveform_array = d_waveform.sample().as(c32);
    d_num_samp = waveform_array.elements();
    d_data.reset(reinterpret_cast<gr_complex*>(waveform_array.host<af::cfloat>()));

    // Set up metadata
    d_meta = pmt::make_dict();
    // Global object
    d_global = pmt::make_dict();
    d_global = pmt::dict_add(
        d_global, PMT_SAMPLE_RATE, pmt::from_double(d_waveform.samp_rate()));

    // Annotations array
    d_annotations = pmt::make_dict();
    d_annotations = pmt::dict_add(d_annotations, PMT_LABEL, pmt::intern("lfm"));
    d_annotations = pmt::dict_add(
        d_annotations, PMT_BANDWIDTH, pmt::from_double(d_waveform.bandwidth()));
    d_annotations = pmt::dict_add(
        d_annotations, PMT_DURATION, pmt::from_double(d_waveform.pulse_width()));

    // Add the global and annotations field to the array
    d_meta = pmt::dict_add(d_meta, PMT_GLOBAL, d_global);
    d_meta = pmt::dict_add(d_meta, PMT_ANNOTATIONS, d_annotations);

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


} /* namespace plasma */
} /* namespace gr */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pcfm_source_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

// #pragma message("set the following appropriately and remove this warning")
// using input_type = float;
// #pragma message("set the following appropriately and remove this warning")
// using output_type = float;
pcfm_source::sptr pcfm_source::make(std::vector<double>& code,
                                    std::vector<double>& filter,
                                    double samp_rate)
{
    return gnuradio::make_block_sptr<pcfm_source_impl>(code, filter, samp_rate);
}


/*
 * The private constructor
 */
pcfm_source_impl::pcfm_source_impl(std::vector<double>& code,
                                   std::vector<double>& filter,
                                   double samp_rate)
    : gr::block("pcfm_source",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_code(Eigen::Map<Eigen::ArrayXd>(code.data(), code.size())),
      d_filter(Eigen::Map<Eigen::ArrayXd>(filter.data(), filter.size())),
      d_samp_rate(samp_rate)
{
    d_waveform = ::plasma::PCFMWaveform(d_code, d_filter, d_samp_rate, 0);
    d_data = d_waveform.sample().cast<gr_complex>();

    d_out_port = PMT_OUT;
    message_port_register_out(d_out_port);
}

/*
 * Our virtual destructor.
 */
pcfm_source_impl::~pcfm_source_impl() {}

bool pcfm_source_impl::start()
{
    d_finished = false;
    d_meta = pmt::make_dict();
    d_meta = pmt::dict_add(
        d_meta, PMT_SAMPLE_RATE, pmt::from_double(d_waveform.samp_rate()));
    pmt::pmt_t data = pmt::init_c32vector(d_data.size(), d_data.data());
    message_port_pub(d_out_port, pmt::cons(d_meta, data));

    return block::start();
}


} /* namespace plasma */
} /* namespace gr */

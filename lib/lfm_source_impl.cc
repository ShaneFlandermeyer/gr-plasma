/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include "lfm_source_impl.h"

namespace gr {
  namespace plasma {

    #pragma message("set the following appropriately and remove this warning")
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
    lfm_source_impl::lfm_source_impl(double bandwidth, double pulse_width, double prf, double samp_rate)
      : gr::sync_block("lfm_source",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(output_type)))
    {}

    /*
     * Our virtual destructor.
     */
    lfm_source_impl::~lfm_source_impl()
    {
    }

    int
    lfm_source_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      auto out = static_cast<output_type*>(output_items[0]);

      #pragma message("Implement the signal processing in your block and remove this warning")
      // Do <+signal processing+>

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace plasma */
} /* namespace gr */

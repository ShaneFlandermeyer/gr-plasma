/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_LFM_SOURCE_IMPL_H
#define INCLUDED_PLASMA_LFM_SOURCE_IMPL_H

#include <gnuradio/plasma/lfm_source.h>

namespace gr {
  namespace plasma {

    class lfm_source_impl : public lfm_source
    {
     private:
      // Nothing to declare in this block.

     public:
      lfm_source_impl(double bandwidth, double pulse_width, double prf, double samp_rate);
      ~lfm_source_impl();

      // Where all the action really happens
      int work(
              int noutput_items,
              gr_vector_const_void_star &input_items,
              gr_vector_void_star &output_items
      );
    };

  } // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_LFM_SOURCE_IMPL_H */

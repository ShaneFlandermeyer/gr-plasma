/* -*- c++ -*- */
/*
 * Copyright 2024 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_SLICER_IMPL_H
#define INCLUDED_PLASMA_PDU_SLICER_IMPL_H

#include <gnuradio/plasma/pdu_slicer.h>

namespace gr {
  namespace plasma {

    class pdu_slicer_impl : public pdu_slicer
    {
     private:
      // Nothing to declare in this block.

     public:
      pdu_slicer_impl(int min_index, int max_index, bool abs_max_index, double samp_rate);
      ~pdu_slicer_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_SLICER_IMPL_H */

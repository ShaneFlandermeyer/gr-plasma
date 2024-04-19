/* -*- c++ -*- */
/*
 * Copyright 2024 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_RANGE_LIMIT_IMPL_H
#define INCLUDED_PLASMA_RANGE_LIMIT_IMPL_H

#include <gnuradio/plasma/range_limit.h>

namespace gr {
  namespace plasma {

    class range_limit_impl : public range_limit
    {
     private:
      // Nothing to declare in this block.

     public:
      range_limit_impl(int range);
      ~range_limit_impl();

      // Where all the action really happens
      void forecast (int noutput_items, gr_vector_int &ninput_items_required);

      int general_work(int noutput_items,
           gr_vector_int &ninput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);

    };

  } // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_LIMIT_IMPL_H */

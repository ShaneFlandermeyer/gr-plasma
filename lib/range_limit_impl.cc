/* -*- c++ -*- */
/*
 * Copyright 2024 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include "range_limit_impl.h"

namespace gr {
  namespace plasma {
    range_limit::sptr range_limit::make(int min_range, int max_range, bool abs_max_range, double multiplier)
    {
      return gnuradio::make_block_sptr<range_limit_impl>(min_range, max_range, abs_max_range, multiplier);
    }

    /*
     * The private constructor
     */
    range_limit_impl::range_limit_impl(int min_range, int max_range, bool abs_max_range, double multiplier)
      : gr::block("range_limit", gr::io_signature::make(0,0,0), gr::io_signature::make(0,0,0)),
        d_min_range(min_range),
        d_max_range(max_range),
        d_abs_max_range(abs_max_range),
        d_multiplier(multiplier)
    {
      d_data = pmt::make_c32vector(1, 0);
      d_meta = pmt::make_dict();

      d_in_port = PMT_IN;
      d_out_port = PMT_OUT;
      message_port_register_in(d_in_port);
      message_port_register_out(d_out_port);
      set_msg_handler(d_in_port, [this](pmt::pmt_t msg) {handle_msg(msg);});
    }

    /*
     * Our virtual destructor.
     */
    range_limit_impl::~range_limit_impl() {}

    void handle_msg(pmt::pmt_t msg){
      
    }

  } /* namespace plasma */
} /* namespace gr */

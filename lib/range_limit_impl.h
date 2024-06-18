/* -*- c++ -*- */
/*
 * Copyright 2024 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_RANGE_LIMIT_IMPL_H
#define INCLUDED_PLASMA_RANGE_LIMIT_IMPL_H

#include <gnuradio/plasma/range_limit.h>
#include <gnuradio/plasma/pmt_constants.h>

namespace gr {
  namespace plasma {

    class range_limit_impl : public range_limit
    {
     private:
      int d_min_range;
      int d_max_range;
      bool d_abs_max_range;
      int d_multiplier;

      pmt::pmt_t d_meta;
      pmt::pmt_t d_min_range_key;
      pmt::pmt_t d_max_range_key;
      pmt::pmt_t d_range_mult_key;
      pmt::pmt_t d_data;

      pmt::pmt_t d_in_port;
      pmt::pmt_t d_out_port;
      void handle_msg(pmt::pmt_t);

     public:
      range_limit_impl(int min_range, int max_range, bool abs_max_range, double multiplier);
      void set_metadata_keys(const std::string& min_range_key, const std::string& max_range_key, const std::string& range_mult_key);
      ~range_limit_impl();
    };

  } // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_LIMIT_IMPL_H */

/* -*- c++ -*- */
/*
 * Copyright 2024 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_RANGE_LIMIT_H
#define INCLUDED_PLASMA_RANGE_LIMIT_H

#include <gnuradio/plasma/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace plasma {

    /*!
     * \brief <+description of block+>
     * \ingroup plasma
     *
     */
    class PLASMA_API range_limit : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<range_limit> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of plasma::range_limit.
       *
       * To avoid accidental use of raw pointers, plasma::range_limit's
       * constructor is in a private implementation
       * class. plasma::range_limit::make is the public interface for
       * creating new instances.
       */
      static sptr make(int min_range=100, int max_range=10000, bool abs_max_range=false, double multiplier=1, double samp_rate=32000);

      virtual void set_metadata_keys(const std::string& min_range_key, const std::string& max_range_key, const std::string& range_mult_key) = 0;
    };

  } // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_RANGE_LIMIT_H */

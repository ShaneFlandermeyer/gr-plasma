/* -*- c++ -*- */
/*
 * Copyright 2024 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_SLICER_H
#define INCLUDED_PLASMA_PDU_SLICER_H

#include <gnuradio/plasma/api.h>
#include <gnuradio/block.h>

namespace gr {
  namespace plasma {

    /*!
     * \brief <+description of block+>
     * \ingroup plasma
     *
     */
    class PLASMA_API pdu_slicer : virtual public gr::block
    {
     public:
      typedef std::shared_ptr<pdu_slicer> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of plasma::pdu_slicer.
       *
       * To avoid accidental use of raw pointers, plasma::pdu_slicer's
       * constructor is in a private implementation
       * class. plasma::pdu_slicer::make is the public interface for
       * creating new instances.
       */
      static sptr make(int min_index, int max_index, bool abs_max_index, double samp_rate);
    };

  } // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_SLICER_H */

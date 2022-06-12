/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_HEAD_H
#define INCLUDED_PLASMA_PDU_HEAD_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief Pass the first nitems PDU messages directly to the output port and
 * block all subsequent PDUs
 * \ingroup plasma
 *
 */
class PLASMA_API pdu_head : virtual public gr::block
{
public:
    typedef std::shared_ptr<pdu_head> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::pdu_head.
     *
     * To avoid accidental use of raw pointers, plasma::pdu_head's
     * constructor is in a private implementation
     * class. plasma::pdu_head::make is the public interface for
     * creating new instances.
     */
    static sptr make(size_t nitems);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_HEAD_H */

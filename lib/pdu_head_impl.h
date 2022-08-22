/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_HEAD_IMPL_H
#define INCLUDED_PLASMA_PDU_HEAD_IMPL_H

#include <gnuradio/plasma/pdu_head.h>
#include <gnuradio/plasma/pmt_constants.h>

namespace gr {
namespace plasma {

class pdu_head_impl : public pdu_head
{
private:
    size_t d_nitems;
    size_t d_nitems_sent;
    

public:
    pdu_head_impl(size_t nitems);
    ~pdu_head_impl();

    void handle_message(const pmt::pmt_t& msg);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_HEAD_IMPL_H */

/* -*- c++ -*- */
/*
 * Copyright 2023 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_SIMULATE_RX_IMPL_H
#define INCLUDED_PLASMA_SIMULATE_RX_IMPL_H

#include <gnuradio/plasma/simulate_rx.h>
#include <gnuradio/plasma/pmt_constants.h>

namespace gr {
namespace plasma {

class simulate_rx_impl : public simulate_rx
{
private:
    // Nothing to declare in this block.
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_out_port;

public:
    simulate_rx_impl(double delay, double scale);

    void handle_msg(pmt::pmt_t msg);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_SIMULATE_RX_IMPL_H */

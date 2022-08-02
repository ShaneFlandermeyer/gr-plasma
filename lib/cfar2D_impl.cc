/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cfar2D_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {


cfar2D::sptr cfar2D::make(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa)
{
    return gnuradio::make_block_sptr<cfar2D_impl>(guard_cells, training_cells, pfa);
}


/*
 * The private constructor
 */
cfar2D_impl::cfar2D_impl(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa)
    : gr::block("cfar2D",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
                outPort(pmt::mp("Output"))
{
    message_port_register_out(outPort);

    message_port_register_in(pmt::mp("Input"));
    set_msg_handler(pmt::mp("Input"),
                    [this](const pmt::pmt_t &msg) {this->recieveMessage(msg);});
}

/*
 * Our virtual destructor.
 */
cfar2D_impl::~cfar2D_impl() {}



} /* namespace plasma */
} /* namespace gr */

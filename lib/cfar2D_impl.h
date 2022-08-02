/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_CFAR2D_IMPL_H
#define INCLUDED_PLASMA_CFAR2D_IMPL_H

#include <gnuradio/plasma/cfar2D.h>

namespace gr {
namespace plasma {

class cfar2D_impl : public cfar2D
{
private:
    const pmt::pmt_t outPort;

    void recieveMessage(const pmt::pmt_t &msg);

public:
    cfar2D_impl(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa);
    ~cfar2D_impl();

};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_CFAR2D_IMPL_H */

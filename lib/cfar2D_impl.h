/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_CFAR2D_IMPL_H
#define INCLUDED_PLASMA_CFAR2D_IMPL_H

#include <plasma_dsp/cfar2d.h>
#include <gnuradio/plasma/cfar2D.h>

namespace gr {
namespace plasma {

class cfar2D_impl : public cfar2D
{
private:
    const pmt::pmt_t outPort;

    //Parameters for CFAR Object
    std::array<size_t,2> guard_win;
    std::array<size_t,2> train_win;
    float _pfa;

    ::plasma::CFARDetector2D cfarTemp;

    void recieveMessage(const pmt::pmt_t &msg);

public:
    cfar2D_impl(std::vector<int> &guard_cells, std::vector<int> &training_cells, float pfa);
    ~cfar2D_impl();

};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_CFAR2D_IMPL_H */

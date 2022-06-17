/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H
#define INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H

#include <gnuradio/plasma/waveform_controller.h>
#include <uhd/usrp/multi_usrp.hpp>

namespace gr {
namespace plasma {

class waveform_controller_impl : public waveform_controller
{
private:
    // Nothing to declare in this block.
    double d_prf;
    double d_samp_rate;
    size_t d_num_samp_pri;
    size_t d_num_samp_waveform;
    std::vector<gr_complex> d_data;
    std::atomic<bool> d_updated;
    std::atomic<bool> d_finished;

    /**
     * @brief Message handler function
     * 
     * @param msg Message in the input port of the block. If this is a PDU, the
     * data is extracted and used to update the waveform vector, and the
     * metadata is added as stream tags on the first sample of each waveform.
     * A dictionary message can also be sent to update the parameters of the
     * emission. The following keys may be used
     *  - prf: Updates the PRF of the waveform
     * 
     */
    void handle_message(const pmt::pmt_t& msg);

public:
    waveform_controller_impl(double prf, double samp_rate);
    ~waveform_controller_impl();

    bool start() override;
    bool stop() override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H */

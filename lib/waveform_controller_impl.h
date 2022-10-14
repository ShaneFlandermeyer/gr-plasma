/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H
#define INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H

#include <gnuradio/plasma/pmt_constants.h>
#include <gnuradio/plasma/waveform_controller.h>
#include <uhd/usrp/multi_usrp.hpp>

namespace gr {
namespace plasma {

class waveform_controller_impl : public waveform_controller
{
private:
    // Message handling
    pmt::pmt_t d_annotations;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_out_port;

    // Metadata keys
    pmt::pmt_t d_prf_key;
    pmt::pmt_t d_sample_rate_key;

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

    void init_meta_dict(std::string prf_key, std::string samp_rate_key) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_WAVEFORM_CONTROLLER_IMPL_H */

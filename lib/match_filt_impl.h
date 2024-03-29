/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_MATCH_FILT_IMPL_H
#define INCLUDED_PLASMA_MATCH_FILT_IMPL_H

#include <gnuradio/plasma/match_filt.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <arrayfire.h>


namespace gr {
namespace plasma {

class match_filt_impl : public match_filt
{
private:
    af::array d_match_filt;
    af::Backend d_backend;
    size_t d_num_pulse_cpi;
    size_t d_msg_queue_depth;
    
    pmt::pmt_t d_meta;
    pmt::pmt_t d_n_pulse_cpi_key;

    pmt::pmt_t d_data;
    pmt::pmt_t d_tx_port;
    pmt::pmt_t d_rx_port;
    pmt::pmt_t d_out_port;
    void handle_tx_msg(pmt::pmt_t);
    void handle_rx_msg(pmt::pmt_t);

public:
    match_filt_impl(size_t num_pulse_cpi);
    ~match_filt_impl();

    void set_msg_queue_depth(size_t) override;
    void set_backend(Device::Backend) override;
    void set_metadata_keys(const std::string& n_pulse_cpi_key) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_MATCH_FILT_IMPL_H */

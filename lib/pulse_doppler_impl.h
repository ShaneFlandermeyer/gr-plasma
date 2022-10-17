/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PULSE_DOPPLER_IMPL_H
#define INCLUDED_PLASMA_PULSE_DOPPLER_IMPL_H

#include <gnuradio/plasma/pmt_constants.h>
#include <gnuradio/plasma/pulse_doppler.h>
#include <plasma_dsp/fft.h>

namespace gr {
namespace plasma {

class pulse_doppler_impl : public pulse_doppler
{
private:
    af::array d_match_filt;
    af::Backend d_backend;
    size_t d_msg_queue_depth;
    int d_num_pulse_cpi;
    int d_fftsize;

    pmt::pmt_t d_tx_port;
    pmt::pmt_t d_rx_port;
    pmt::pmt_t d_out_port;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_data;
    // Metadata keys
    pmt::pmt_t d_doppler_fft_size_key;

    void handle_tx_msg(pmt::pmt_t);
    void handle_rx_msg(pmt::pmt_t);

public:
    pulse_doppler_impl(int num_pulse_cpi, int doppler_fft_size);
    ~pulse_doppler_impl();

    void set_msg_queue_depth(size_t) override;
    void set_backend(Device::Backend) override;
    void init_meta_dict(std::string doppler_fft_size_key) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PULSE_DOPPLER_IMPL_H */
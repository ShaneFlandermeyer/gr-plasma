/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H
#define INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H

#include <gnuradio/plasma/device.h>
#include <gnuradio/plasma/doppler_processing.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <plasma_dsp/fft.h>

namespace gr {
namespace plasma {

class doppler_processing_impl : public doppler_processing
{
private:
    size_t d_num_pulse_cpi;
    size_t d_fftsize;
    size_t d_queue_depth;

    void handle_msg(pmt::pmt_t msg);

    pmt::pmt_t d_out_port;
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_data;
    pmt::pmt_t d_meta;
    pmt::pmt_t d_n_pulse_cpi_key;
    pmt::pmt_t d_doppler_fft_size_key;

    af::Backend d_backend;

public:
    doppler_processing_impl(size_t num_pulse_cpi, size_t nfft);
    ~doppler_processing_impl();

    void set_metadata_keys(const std::string& n_pulse_cpi_key,
                           const std::string& doppler_fft_size_key);
    void set_msg_queue_depth(size_t) override;
    void set_backend(Device::Backend) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H */

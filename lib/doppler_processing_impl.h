/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H
#define INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H

#include <gnuradio/plasma/doppler_processing.h>
#include <Eigen/Dense>
#include <gnuradio/fft/fft.h>
#include <gnuradio/fft/fft_shift.h>

namespace gr {
namespace plasma {

class doppler_processing_impl : public doppler_processing
{
private:
    size_t d_num_pulse_cpi;
    size_t d_nfft;
    Eigen::ArrayXcf d_in_data;

    void handle_msg(pmt::pmt_t msg);

public:
    doppler_processing_impl(size_t num_pulse_cpi, size_t nfft);
    ~doppler_processing_impl();
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_DOPPLER_PROCESSING_IMPL_H */

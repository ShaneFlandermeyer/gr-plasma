/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PULSE_TO_CPI_IMPL_H
#define INCLUDED_PLASMA_PULSE_TO_CPI_IMPL_H

#include <gnuradio/plasma/pulse_to_cpi.h>
#include <gnuradio/plasma/pmt_constants.h>

namespace gr {
namespace plasma {

class pulse_to_cpi_impl : public pulse_to_cpi
{
private:
    // Metadata keys
    pmt::pmt_t d_n_pulse_cpi_key;
    pmt::pmt_t d_in_port;
    pmt::pmt_t d_out_port;
    pmt::pmt_t d_meta;

    std::vector<gr_complex> d_data;
    size_t d_n_pulse_cpi;
    size_t d_pulse_count;


public:
    void handle_msg(pmt::pmt_t msg);
    pulse_to_cpi_impl(size_t n_pulse_cpi);
    ~pulse_to_cpi_impl();

    void init_meta_dict(std::string n_pulse_cpi_key) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PULSE_TO_CPI_IMPL_H */

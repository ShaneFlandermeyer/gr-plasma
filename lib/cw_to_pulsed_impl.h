/* -*- c++ -*- */
/*
 * Copyright 2023 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_CW_TO_PULSED_IMPL_H
#define INCLUDED_PLASMA_CW_TO_PULSED_IMPL_H

#include <gnuradio/plasma/cw_to_pulsed.h>
#include <gnuradio/plasma/pmt_constants.h>

namespace gr {
namespace plasma {

class cw_to_pulsed_impl : public cw_to_pulsed
{
private:
    double d_prf;
    double d_sample_rate;

    // TODO: Make this key user-configurable
    pmt::pmt_t d_nonzero_key = pmt::string_to_symbol("n_nonzero");
    pmt::pmt_t d_sample_rate_key;
    pmt::pmt_t d_prf_key;

    pmt::pmt_t d_in_port;
    pmt::pmt_t d_out_port;

public:
    cw_to_pulsed_impl(double prf, double samp_rate);

    void handle_msg(pmt::pmt_t msg);
    void parse_meta(pmt::pmt_t meta);

    void init_meta_dict(const std::string& sample_rate_key,
                                const std::string& prf_key);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_CW_TO_PULSED_IMPL_H */

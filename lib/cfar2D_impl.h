/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_CFAR2D_IMPL_H
#define INCLUDED_PLASMA_CFAR2D_IMPL_H

#include <gnuradio/plasma/cfar2D.h>
#include <gnuradio/plasma/pmt_constants.h>
#include <plasma_dsp/cfar2d.h>

namespace gr {
namespace plasma {

class cfar2D_impl : public cfar2D
{
private:
    // Metadata keys
    pmt::pmt_t d_detection_indices_key;
    pmt::pmt_t d_n_detections_key;
    pmt::pmt_t d_n_pulse_cpi_key;

    // Port names
    const pmt::pmt_t d_in_port;
    const pmt::pmt_t d_out_port;

    af::Backend d_backend;

    // Parameters for CFAR Object
    std::array<size_t, 2> d_guard_win_size;
    std::array<size_t, 2> d_train_win_size;
    double d_pfa;
    size_t d_num_pulse_cpi;
    size_t d_msg_queue_depth;
    ::plasma::CFARDetector2D detector;

    void handle_message(const pmt::pmt_t& msg);

public:
    cfar2D_impl(std::vector<int>& guard_win_size,
                std::vector<int>& train_win_size,
                double pfa,
                size_t num_pulse_cpi);
    ~cfar2D_impl();

    void set_msg_queue_depth(size_t) override;
    void set_backend(Device::Backend) override;

    void set_metadata_keys(std::string detction_indices_key,
                           std::string n_detections_key,
                           std::string n_pulse_cpi_key) override;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_CFAR2D_IMPL_H */

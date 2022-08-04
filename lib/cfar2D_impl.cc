/* -*- c++ -*- */
/*
 * Copyright 2022 Avery Moates.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "arrayfire.h"
#include "cfar2D_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {


cfar2D::sptr cfar2D::make(std::vector<int>& guard_win_size,
                          std::vector<int>& train_win_size,
                          double pfa,
                          size_t num_pulse_cpi)
{
    return gnuradio::make_block_sptr<cfar2D_impl>(
        guard_win_size, train_win_size, pfa, num_pulse_cpi);
}


/*
 * The private constructor
 */
cfar2D_impl::cfar2D_impl(std::vector<int>& guard_win_size,
                         std::vector<int>& train_win_size,
                         double pfa,
                         size_t num_pulse_cpi)
    : gr::block(
          "cfar2D", gr::io_signature::make(0, 0, 0), gr::io_signature::make(0, 0, 0)),
      d_in_port(PMT_IN),
      d_out_port(PMT_OUT),
      d_num_pulse_cpi(num_pulse_cpi)
{
    // Set up the CFAR detector objects
    std::copy(guard_win_size.begin(), guard_win_size.end(), d_guard_win_size.begin());
    std::copy(train_win_size.begin(), train_win_size.end(), d_train_win_size.begin());
    d_pfa = pfa;
    detector = ::plasma::CFARDetector2D(d_guard_win_size, d_train_win_size, d_pfa);

    // Message handling
    message_port_register_out(d_out_port);
    message_port_register_in(d_in_port);
    set_msg_handler(d_in_port,
                    [this](const pmt::pmt_t& msg) { this->handle_message(msg); });
}

/*
 * Our virtual destructor.
 */
cfar2D_impl::~cfar2D_impl() {}

void cfar2D_impl::handle_message(const pmt::pmt_t& msg)
{
    // Parse the input message
    pmt::pmt_t samples;
    pmt::pmt_t meta = pmt::make_dict();
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
        meta = pmt::dict_update(meta, pmt::car(msg));
    } else if (pmt::is_uniform_vector(msg)) {
        samples = pmt::cdr(msg);
    } else {
        GR_LOG_WARN(d_logger, "Message must be a PDU or uniform vector")
        return;
    }

    // Convert the input data to a 2D range-doppler map
    size_t n = pmt::length(samples);
    size_t io(0);
    int ncol = d_num_pulse_cpi;
    int nrow = n / ncol;
    const gr_complex* in = pmt::c32vector_elements(samples, io);
    af::array rdm(af::dim4(nrow, ncol), reinterpret_cast<const af::cfloat*>(in));

    // Run the CFAR detector on the magnitude squared of the input data
    rdm = af::pow(af::abs(rdm), 2);
    DetectionReport results = detector.detect(rdm);

    // Convert the resulting array of linear indices to a PMT object
    int* ind_ptr = results.indices.as(s32).host<int>();
    pmt::pmt_t indices = pmt::init_s32vector(
        results.indices.elements(),
        std::vector<int>(ind_ptr, ind_ptr + results.indices.elements()));
    delete ind_ptr;

    // Add detection metadata (directly passing the input data)
    meta = pmt::dict_add(meta, pmt::intern("indices"), indices);
    meta = pmt::dict_add(
        meta, pmt::intern("num_detections"), pmt::from_long(results.num_detections));

    message_port_pub(d_out_port, pmt::cons(meta, samples));
}


} /* namespace plasma */
} /* namespace gr */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pulse_doppler_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

pulse_doppler::sptr pulse_doppler::make(int num_pulse_cpi, int doppler_fft_size)
{
    return gnuradio::make_block_sptr<pulse_doppler_impl>(num_pulse_cpi, doppler_fft_size);
}


/*
 * The private constructor
 */
pulse_doppler_impl::pulse_doppler_impl(int num_pulse_cpi, int doppler_fft_size)
    : gr::block("pulse_doppler",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      num_pulse_cpi(num_pulse_cpi),
      nfft(doppler_fft_size)
{
    tx_port = PMT_TX;
    rx_port = PMT_RX;
    out_port = PMT_OUT;
    message_port_register_in(tx_port);
    message_port_register_in(rx_port);
    message_port_register_out(out_port);
    set_msg_handler(tx_port, [this](pmt::pmt_t msg) { handle_tx_msg(msg); });
    set_msg_handler(rx_port, [this](pmt::pmt_t msg) { handle_rx_msg(msg); });
}


/*
 * Our virtual destructor.
 */
pulse_doppler_impl::~pulse_doppler_impl() {}

void pulse_doppler_impl::handle_tx_msg(pmt::pmt_t msg)
{
    if (this->nmsgs(tx_port) > msg_queue_depth)
        return;
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
        meta = pmt::dict_update(meta, pmt::car(msg));
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    size_t n = pmt::length(samples);

    const gr_complex* tx_samples = pmt::c32vector_elements(samples, n);

    // Create the matched filter
    match_filt = af::array(af::dim4(n), reinterpret_cast<const af::cfloat*>(tx_samples));
    match_filt = af::conjg(match_filt);
    match_filt = af::flip(match_filt, 0);
}

void pulse_doppler_impl::handle_rx_msg(pmt::pmt_t msg)
{
    pmt::pmt_t rx_samples;
    if (this->nmsgs(rx_port) > msg_queue_depth or match_filt.elements() == 0) {
        return;
    }
    // Get a copy of the input samples
    if (pmt::is_pdu(msg)) {
        rx_samples = pmt::cdr(msg);
        meta = pmt::dict_update(meta, pmt::car(msg));
    } else if (pmt::is_uniform_vector(msg)) {
        rx_samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }

    // Compute matrix and vector dimensions
    size_t num_samples_cpi = pmt::length(rx_samples);
    size_t num_fast_time_samples = num_samples_cpi / num_pulse_cpi;
    size_t num_rdm_samples = nfft * (num_fast_time_samples + match_filt.elements() - 1);

    // Get input and output data
    const gr_complex* in = pmt::c32vector_elements(rx_samples, num_samples_cpi);


    // Apply the matched filter to each column
    af::array rdm(af::dim4(num_fast_time_samples, num_pulse_cpi),
                  reinterpret_cast<const af::cfloat*>(in));
    rdm = af::convolve1(rdm, match_filt, AF_CONV_EXPAND, AF_CONV_AUTO);

    // Do a doppler FFT for each range bin
    rdm = rdm.T();
    rdm = af::fftNorm(rdm, 1.0, nfft);
    rdm = ::plasma::fftshift(rdm, 0);
    rdm = rdm.T();

    pmt::pmt_t rdm_pmt = pmt::make_c32vector(num_rdm_samples, 0);
    gr_complex* rdm_data = pmt::c32vector_writable_elements(rdm_pmt, num_rdm_samples);
    rdm.host(rdm_data);

    message_port_pub(out_port, pmt::cons(meta, rdm_pmt));
    meta = pmt::make_dict();
}

void pulse_doppler_impl::set_msg_queue_depth(size_t depth) { msg_queue_depth = depth; }

void pulse_doppler_impl::set_backend(Device::Backend backend)
{
    // switch (backend) {
    // case Device::CPU:
    //     backend = AF_BACKEND_CPU;
    //     break;
    // case Device::CUDA:
    //     backend = AF_BACKEND_CUDA;
    //     break;
    // case Device::OPENCL:
    //     backend = AF_BACKEND_OPENCL;
    //     break;
    // default:
    //     backend = AF_BACKEND_DEFAULT;
    //     break;
    // }
    // af::setBackend(backend);
}

void pulse_doppler_impl::init_meta_dict(std::string doppler_fft_size_key)
{
    this->doppler_fft_size_key = pmt::intern(doppler_fft_size_key);
    meta = pmt::make_dict();
    meta = pmt::dict_add(meta, this->doppler_fft_size_key, pmt::from_long(nfft));
}

} /* namespace plasma */
} /* namespace gr */
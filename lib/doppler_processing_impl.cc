/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "doppler_processing_impl.h"
#include <gnuradio/io_signature.h>
#include <arrayfire.h>
#include <chrono>

#define fftshift(in) shift(in, in.dims(0) / 2, 0)
#define ifftshift(in) shift(in, (in.dims(0) + 1) / 2, (in.dims(1) + 1) / 2)
namespace gr {
namespace plasma {

doppler_processing::sptr doppler_processing::make(size_t num_pulse_cpi, size_t nfft)
{
    return gnuradio::make_block_sptr<doppler_processing_impl>(num_pulse_cpi, nfft);
}


/*
 * The private constructor
 */
doppler_processing_impl::doppler_processing_impl(size_t num_pulse_cpi, size_t nfft)
    : gr::block("doppler_processing",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_num_pulse_cpi(num_pulse_cpi)
{
    d_in_port = PMT_IN;
    d_out_port = PMT_OUT;
    d_meta = pmt::make_dict();
    d_data = pmt::make_c32vector(0, 0);
    message_port_register_in(d_in_port);
    message_port_register_out(d_out_port);
    set_msg_handler(d_in_port, [this](pmt::pmt_t msg) { handle_msg(msg); });
}

/*
 * Our virtual destructor.
 */
doppler_processing_impl::~doppler_processing_impl() {}

void doppler_processing_impl::handle_msg(pmt::pmt_t msg)
{
    
    if (this->nmsgs(PMT_IN) > d_queue_depth) {
        return;
    }

    // Read the input PDU
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
        d_meta = pmt::dict_update(d_meta, pmt::car(msg));
        d_meta = pmt::dict_add(d_meta, PMT_DOPPLER_FFT_SIZE, pmt::from_long(d_fftsize));
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }

    // Get pointers to the input and output arrays
    size_t n = pmt::length(samples);
    size_t io(0);
    if (pmt::length(d_data) != n)
        d_data = pmt::make_c32vector(n, 0);
    gr_complex* in = pmt::c32vector_writable_elements(samples, io);
    gr_complex* out = pmt::c32vector_writable_elements(d_data, io);
    int nrow = n / d_num_pulse_cpi;
    int ncol = d_num_pulse_cpi;

    // Take an FFT across each row of the matrix to form a range-doppler map
    af::sync();
    af::array rdm(af::dim4(nrow,ncol),c32);
    rdm.write(reinterpret_cast<af::cfloat*>(in), n * sizeof(gr_complex));
    // The FFT function transforms each column of the input matrix by default,
    // so we need to transpose it to do the FFT across rows.
    rdm = rdm.T();
    af::fftInPlace(rdm);
    rdm = fftshift(rdm);
    rdm = rdm.T();
    rdm.host(out);

    // Send the data as a message
    message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
    d_meta = pmt::make_dict();
}

void doppler_processing_impl::set_msg_queue_depth(size_t depth) { d_queue_depth = depth; }

void doppler_processing_impl::set_backend(Device::Backend backend)
{
    switch (backend) {
    case Device::CPU:
        d_backend = AF_BACKEND_CPU;
        break;
    case Device::CUDA:
        d_backend = AF_BACKEND_CUDA;
        break;
    case Device::OPENCL:
        d_backend = AF_BACKEND_OPENCL;
        break;
    default:
        d_backend = AF_BACKEND_DEFAULT;
        break;
    }
    af::setBackend(d_backend);
    
}
} /* namespace plasma */
} /* namespace gr */

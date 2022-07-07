/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "doppler_processing_impl.h"
#include <gnuradio/io_signature.h>
#include <chrono>

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
      d_num_pulse_cpi(num_pulse_cpi),
      d_shift(nfft)
{
    fftresize(nfft);

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

bool doppler_processing_impl::start()
{
    d_finished = false;
    return block::start();
}

bool doppler_processing_impl::stop()
{
    d_finished = true;
    return block::stop();
}

void doppler_processing_impl::handle_msg(pmt::pmt_t msg)
{
    if (this->nmsgs(PMT_IN) > d_queue_depth) {
        return;
    }

    // Read the input PDU and map it to an eigen array
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
        d_meta = pmt::dict_update(d_meta, pmt::car(msg));
        d_meta = pmt::dict_add(
            d_meta, PMT_DOPPLER_FFT_SIZE, pmt::from_long(d_fftsize));
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    // Resize the output data vector if necessary
    size_t n = pmt::length(samples);
    size_t nout = n * (d_fftsize / d_num_pulse_cpi);
    if (pmt::length(d_data) != nout)
        d_data = pmt::make_c32vector(nout, 0);
    Eigen::Map<Eigen::ArrayXXcf> data(pmt::c32vector_writable_elements(samples, n),
                                      n / d_num_pulse_cpi,
                                      d_num_pulse_cpi);
    Eigen::Map<Eigen::ArrayXXcf> out(
        pmt::c32vector_writable_elements(d_data, nout), nout / d_fftsize, d_fftsize);

    // Do an fft and fftshift
    Eigen::ArrayXcf tmp;
    for (auto irow = 0; irow < data.rows(); irow++) {
        tmp = data.row(irow);
        memcpy(d_fwd->get_inbuf(), tmp.data(), data.cols() * sizeof(gr_complex));
        for (size_t i = data.cols(); i < d_fftsize; i++)
            d_fwd->get_inbuf()[i] = 0;
        d_fwd->execute();
        d_shift.shift(d_fwd->get_outbuf(), d_fftsize);

        out.row(irow) = Eigen::Map<Eigen::ArrayXcf>(d_fwd->get_outbuf(), d_fftsize);
    }
    // Send the data as a message
    message_port_pub(d_out_port, pmt::cons(d_meta, d_data));
    d_meta = pmt::make_dict();
}

void doppler_processing_impl::fftresize(size_t size)
{
    gr::thread::scoped_lock lock(d_setlock);

    if (size != d_fftsize) {
        // Re-initialize the plans
        d_fftsize = size;
        d_fwd = std::make_unique<fft::fft_complex_fwd>(size);
        d_shift.resize(size);
    }
}

void doppler_processing_impl::set_msg_queue_depth(size_t depth) { d_queue_depth = depth; }
} /* namespace plasma */
} /* namespace gr */

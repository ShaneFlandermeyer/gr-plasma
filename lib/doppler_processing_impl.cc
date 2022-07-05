/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "doppler_processing_impl.h"
#include <gnuradio/io_signature.h>

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
    message_port_register_in(pmt::mp("in"));
    message_port_register_out(pmt::mp("out"));
    set_msg_handler(pmt::mp("in"), [this](pmt::pmt_t msg) { handle_msg(msg); });
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
    d_processing_thread.interrupt();
    return block::stop();
}

void doppler_processing_impl::handle_msg(pmt::pmt_t msg)
{
    if (d_finished) {
        return;
    }
    pmt::pmt_t samples;
    if (pmt::is_pdu(msg)) {
        samples = pmt::cdr(msg);
    } else if (pmt::is_uniform_vector(msg)) {
        samples = msg;
    } else {
        GR_LOG_WARN(d_logger, "Invalid message type")
        return;
    }
    size_t n = pmt::length(samples);
    gr_complex* inptr = pmt::c32vector_writable_elements(samples, n);
    Eigen::ArrayXXcf in =
        Eigen::Map<Eigen::ArrayXXcf>(inptr, n / d_num_pulse_cpi, d_num_pulse_cpi);

    d_processing_thread.join();
    d_processing_thread = gr::thread::thread([this, in] { process_data(in); });
}

void doppler_processing_impl::process_data(const Eigen::ArrayXXcf& data)
{
    Eigen::ArrayXXcf range_dopp = Eigen::ArrayXXcf(data.rows(), d_fftsize);
    for (auto i = 0; i < data.rows(); i++) {
        Eigen::ArrayXcf tmp = data.row(i);
        memcpy(d_fwd->get_inbuf(), tmp.data(), tmp.size() * sizeof(gr_complex));
        for (size_t i = tmp.size(); i < d_fftsize; i++)
            d_fwd->get_inbuf()[i] = 0;
        d_fwd->execute();
        d_shift.shift(d_fwd->get_outbuf(), d_fwd->outbuf_length());

        // Copy the output into the range doppler map object
        std::vector<gr_complex> vec(d_fwd->get_outbuf(),
                                    d_fwd->get_outbuf() + d_fwd->outbuf_length());
        range_dopp.row(i) =
            Eigen::Map<Eigen::ArrayXcf, Eigen::Aligned>(vec.data(), vec.size());
    }
    // Send the data as a message
    pmt::pmt_t out = pmt::init_c32vector(range_dopp.size(), range_dopp.data());
    message_port_pub(pmt::mp("out"), pmt::cons(pmt::make_dict(), out));
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
} /* namespace plasma */
} /* namespace gr */

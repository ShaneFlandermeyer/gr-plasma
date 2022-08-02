/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pcfm_source_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

pcfm_source::sptr
pcfm_source::make(PhaseCode::Code code, int n, int over, double samp_rate)
{
    return gnuradio::make_block_sptr<pcfm_source_impl>(code, n, over, samp_rate);
}


/*
 * The private constructor
 */
pcfm_source_impl::pcfm_source_impl(PhaseCode::Code code,
                                   int n,
                                   int over,
                                   double samp_rate)
    : gr::block("pcfm_source",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_samp_rate(samp_rate)
{
    std::vector<double> codevec = PhaseCode::generate_code(code, n);
    d_code_class = PhaseCode::code_string(code);
    // Convert the code and filter to arrayfire arrays
    d_code = af::array(codevec.size(), f64);
    d_code.write(codevec.data(), codevec.size() * sizeof(double));
    d_filter = af::constant(1, over);
    // Generate the waveform
    d_waveform = ::plasma::PCFMWaveform(d_code, d_filter,0, d_samp_rate);
    af::array waveform_array = d_waveform.sample().as(c32);
    d_data.reset(reinterpret_cast<gr_complex*>(waveform_array.host<af::cfloat>()));
    d_num_samp = waveform_array.elements();

    // Set up metadata
    d_meta = pmt::make_dict();
    // Global object
    d_global = pmt::make_dict();
    d_global = pmt::dict_add(
        d_global, PMT_SAMPLE_RATE, pmt::from_double(d_waveform.samp_rate()));

    // Annotations array
    d_annotations = pmt::make_dict();
    d_annotations = pmt::dict_add(d_annotations, PMT_LABEL, pmt::intern("pcfm"));
    d_annotations = pmt::dict_add(
        d_annotations, PMT_DURATION, pmt::from_double(d_waveform.pulse_width()));
    d_annotations =
        pmt::dict_add(d_annotations, PMT_PHASE_CODE_CLASS, pmt::intern(d_code_class));
    d_annotations =
        pmt::dict_add(d_annotations, PMT_NUM_PHASE_CODE_CHIPS, pmt::from_long(n));


    // Add the global and annotations field to the array
    d_meta = pmt::dict_add(d_meta, PMT_GLOBAL, d_global);
    d_meta = pmt::dict_add(d_meta, PMT_ANNOTATIONS, d_annotations);

    d_out_port = PMT_OUT;
    message_port_register_out(d_out_port);
}

/*
 * Our virtual destructor.
 */
pcfm_source_impl::~pcfm_source_impl() {}

bool pcfm_source_impl::start()
{
    d_finished = false;
    pmt::pmt_t data = pmt::init_c32vector(d_num_samp, d_data.get());
    message_port_pub(d_out_port, pmt::cons(d_meta, data));

    return block::start();
}


} /* namespace plasma */
} /* namespace gr */

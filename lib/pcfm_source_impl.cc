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
    d_waveform = ::plasma::PCFMWaveform(d_code, d_filter, d_samp_rate);
    af::array waveform_array = d_waveform.sample().as(c32);
    d_data.reset(reinterpret_cast<gr_complex*>(waveform_array.host<af::cfloat>()));
    d_n_samp = waveform_array.elements();
    d_n_chips = n;
   
}

/*
 * Our virtual destructor.
 */
pcfm_source_impl::~pcfm_source_impl() {}

bool pcfm_source_impl::start()
{
    d_finished = false;
    pmt::pmt_t data = pmt::init_c32vector(d_n_samp, d_data.get());
    message_port_pub(d_out_port, pmt::cons(d_meta, data));

    return block::start();
}

void pcfm_source_impl::set_metadata_keys(const std::string& label_key,
                                         const std::string& phase_code_class_key,
                                         const std::string& n_phase_code_chips_key,
                                         const std::string& duration_key,
                                         const std::string& sample_rate_key)
{
    d_label_key = pmt::intern(label_key);
    d_phase_code_class_key = pmt::intern(phase_code_class_key);
    d_n_phase_code_chips_key = pmt::intern(n_phase_code_chips_key);
    d_duration_key = pmt::intern(duration_key);
    d_sample_rate_key = pmt::intern(sample_rate_key);

    // Set up metadata
    d_meta = pmt::make_dict();

    d_meta = pmt::dict_add(
        d_meta, d_sample_rate_key, pmt::from_double(d_waveform.samp_rate()));

    d_meta = pmt::dict_add(d_meta, d_label_key, pmt::intern("pcfm"));
    d_meta =
        pmt::dict_add(d_meta, d_duration_key, pmt::from_double(d_waveform.pulse_width()));
    d_meta = pmt::dict_add(d_meta, d_phase_code_class_key, pmt::intern(d_code_class));
    d_meta = pmt::dict_add(d_meta, d_n_phase_code_chips_key, pmt::from_long(d_n_chips));

    d_out_port = PMT_OUT;
    message_port_register_out(d_out_port);
}


} /* namespace plasma */
} /* namespace gr */

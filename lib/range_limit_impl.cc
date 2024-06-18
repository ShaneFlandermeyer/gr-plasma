/* -*- c++ -*- */
/*
 * Copyright 2024 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include <cmath>
#include "range_limit_impl.h"

namespace gr {
  namespace plasma {
    range_limit::sptr range_limit::make(int min_range, int max_range, bool abs_max_range, double multiplier, double samp_rate)
    {
      return gnuradio::make_block_sptr<range_limit_impl>(min_range, max_range, abs_max_range, multiplier, samp_rate);
    }

    /*
     * The private constructor
     */
    range_limit_impl::range_limit_impl(int min_range, int max_range, bool abs_max_range, double multiplier, double samp_rate)
      : gr::block("range_limit", gr::io_signature::make(0,0,0), gr::io_signature::make(0,0,0)),
        d_min_range(min_range),
        d_max_range(max_range),
        d_abs_max_range(abs_max_range),
        d_multiplier(multiplier),
        d_samp_rate(samp_rate)
    {
      d_data = pmt::make_c32vector(1, 0);
      d_meta = pmt::make_dict();

      d_in_port = PMT_IN;
      d_out_port = PMT_OUT;
      message_port_register_in(d_in_port);
      message_port_register_out(d_out_port);
      set_msg_handler(d_in_port, [this](pmt::pmt_t msg) {handle_msg(msg);});
    }

    /*
     * Our virtual destructor.
     */
    range_limit_impl::~range_limit_impl() {}

    void range_limit_impl::handle_msg(pmt::pmt_t msg){
      //Get data, metadata
      pmt::pmt_t samples, meta;
      if (pmt::is_pdu(msg)){
        meta = pmt::car(msg);
        samples = pmt::cdr(msg);
      }
      else if(pmt::is_uniform_vector(msg)){
        samples = msg;
      }
      else{
        GR_LOG_ERROR(d_logger, "Invalid message type");
        return;
      }

      double wf_len;
      //Get waveform length
      if (pmt::dict_has_key(meta, pmt::intern("radar:duration"))){
        wf_len = pmt::to_double(pmt::dict_ref(meta, pmt::intern("radar:duration"), pmt::PMT_NIL));
      }
      else{
        throw std::invalid_argument("Waveform length not defined");
      }

      if (d_min_range < 0 || d_max_range < 0){
        throw std::invalid_argument("Negative Range Entered");
      }

      int min_index, max_index;
      int calc_max_range;
      //Define the min and max index
      auto index_calc = [](double range, double samp_rate) -> int{
        return floor(2*(range/(3e8))*samp_rate);
      };
      min_index = index_calc(d_min_range, d_samp_rate);
      max_index = index_calc(d_max_range, d_samp_rate) + floor(wf_len * d_samp_rate * d_multiplier);
      calc_max_range = d_max_range;

      if(d_abs_max_range || (size_t)max_index > pmt::length(samples) - 1){
        max_index = pmt::length(samples) - 1;
        calc_max_range = floor((max_index*(3e8))/(2*d_samp_rate)); //Correct the max range for metadata
      }

      size_t data_len(0);
      const gr_complex* data = pmt::c32vector_elements(samples, data_len);
      
      int trimmed_len = max_index - min_index + 1;
      pmt::pmt_t samples_trim = pmt::make_c32vector(trimmed_len, 0);
      for (int i = min_index; i <= max_index; i++){
        pmt::c32vector_set(samples_trim, i - min_index, data[i]);
      }

      meta = pmt::dict_add(meta, d_min_range_key, pmt::from_long(d_min_range));
      meta = pmt::dict_add(meta, d_max_range_key, pmt::from_long(calc_max_range));
      meta = pmt::dict_add(meta, d_range_mult_key, pmt::from_long(d_multiplier));
      message_port_pub(d_out_port, pmt::cons(meta, samples_trim));
    }

    void range_limit_impl::set_metadata_keys(const std::string& min_range_key, const std::string& max_range_key, const std::string& range_mult_key){
      d_min_range_key = pmt::intern(min_range_key);
      d_max_range_key = pmt::intern(max_range_key);
      d_range_mult_key = pmt::intern(range_mult_key);
    }

  } /* namespace plasma */
} /* namespace gr */

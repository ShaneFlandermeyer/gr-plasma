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
    range_limit::sptr range_limit::make(int min_range, int max_range, bool abs_max_range, double multiplier)
    {
      return gnuradio::make_block_sptr<range_limit_impl>(min_range, max_range, abs_max_range, multiplier);
    }

    /*
     * The private constructor
     */
    range_limit_impl::range_limit_impl(int min_range, int max_range, bool abs_max_range, double multiplier)
      : gr::block("range_limit", gr::io_signature::make(0,0,0), gr::io_signature::make(0,0,0)),
        d_min_range(min_range),
        d_max_range(max_range),
        d_abs_max_range(abs_max_range),
        d_multiplier(multiplier)
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
        GR_LOG_WARN(d_logger, "Invalid message type");
        return;
      }

      double samp_rate;
      //Get sample rate
      if (pmt::dict_has_key(meta, pmt::intern("core:sample_rate"))){
        samp_rate = pmt::to_double(pmt::dict_ref(meta, pmt::intern("core:sample_rate"), pmt::PMT_NIL));
      }
      else{
        GR_LOG_WARN(d_logger, "Sample rate not defined");
        return;
      }

      double wf_len;
      //Get waveform length
      if (pmt::dict_has_key(meta, pmt::intern("radar:duration"))){
        wf_len = pmt::to_double(pmt::dict_ref(meta, pmt::intern("radar:duration"), pmt::PMT_NIL));
      }
      else{
        GR_LOG_WARN(d_logger, "Waveform length not defined");
        return;
      }


      int min_index, max_index;
      //Define the min and max index
      auto index_calc = [](double range, double samp_rate) -> int{
        return floor(2*(range/(3e8))*samp_rate);
      };
      min_index = index_calc(d_min_range, samp_rate);

      if (d_abs_max_range){
        max_index = pmt::length(samples) - 1;
      }
      else{
        max_index = index_calc(d_max_range, samp_rate) + floor(wf_len * samp_rate * d_multiplier);
      }
      if((size_t)max_index > pmt::length(samples) - 1){
        max_index = pmt::length(samples) - 1;
      }
      //REMOVE THIS
      std::cout << "Min Index: " << min_index << std::endl;
      std::cout << "Max Index: " << max_index << std::endl;
      //END REMOVE
      size_t data_len(0);
      const gr_complex* data = pmt::c32vector_elements(samples, data_len);
      
      int trimmed_len = max_index - min_index + 1;
      pmt::pmt_t samples_trim = pmt::make_c32vector(trimmed_len, 0);
      for (int i = min_index; i <= max_index; i++){
        pmt::c32vector_set(samples_trim, i - min_index, data[i]);
      }

      meta = pmt::dict_add(meta, pmt::intern("radar:min_range"), pmt::from_long(d_min_range));
      meta = pmt::dict_add(meta, pmt::intern("radar:max_range"), pmt::from_long(d_max_range));
      meta = pmt::dict_add(meta, pmt::intern("radar:range_mult"), pmt::from_long(d_multiplier));

      message_port_pub(d_out_port, pmt::cons(meta, samples_trim));
    }

  } /* namespace plasma */
} /* namespace gr */

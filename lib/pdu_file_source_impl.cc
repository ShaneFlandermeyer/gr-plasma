/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "pdu_file_source_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace plasma {

pdu_file_source::sptr pdu_file_source::make(const std::string& data_filename,
                                            const std::string& meta_filename,
                                            int offset,
                                            int length)
{
    return gnuradio::make_block_sptr<pdu_file_source_impl>(
        data_filename, meta_filename, offset, length);
}


/*
 * The private constructor
 */
pdu_file_source_impl::pdu_file_source_impl(const std::string& data_filename,
                                           const std::string& meta_filename,
                                           int offset,
                                           int length)
    : gr::block("pdu_file_source",
                gr::io_signature::make(0, 0, 0),
                gr::io_signature::make(0, 0, 0)),
      d_data_filename(data_filename),
      d_meta_filename(meta_filename),
      d_offset(offset),
      d_length(length)
{
    message_port_register_out(pmt::mp("out"));

    std::vector<gr_complex> data =
        ::plasma::read<gr_complex>(d_data_filename, d_offset, d_length);
    d_data = pmt::init_c32vector(data.size(), data.data());
    // TODO: Load metadata
    d_meta = pmt::make_dict();
}

/*
 * Our virtual destructor.
 */
pdu_file_source_impl::~pdu_file_source_impl() {}

bool pdu_file_source_impl::start()
{
    d_finished = false;
    d_thread = gr::thread::thread([this] { run(); });
    return block::start();
}
bool pdu_file_source_impl::stop()
{
    d_finished = true;
    d_thread.join();
    return block::stop();
}
void pdu_file_source_impl::run()
{
    message_port_pub(pmt::mp("out"), pmt::cons(d_meta, d_data));
}
} /* namespace plasma */
} /* namespace gr */
/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_FILE_SINK_H
#define INCLUDED_PLASMA_PDU_FILE_SINK_H

#include <gnuradio/plasma/api.h>
// #include <gnuradio/sync_block.h>
#include <gnuradio/block.h>

namespace gr {
namespace plasma {

/*!
 * \brief Write PDU data to a file.
 * \ingroup plasma
 *
 */
class PLASMA_API pdu_file_sink : virtual public gr::block
{
public:
    typedef std::shared_ptr<pdu_file_sink> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::pdu_file_sink.
     *
     * To avoid accidental use of raw pointers, plasma::pdu_file_sink's
     * constructor is in a private implementation
     * class. plasma::pdu_file_sink::make is the public interface for
     * creating new instances.
     */
    static sptr make(const std::string& filename);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_FILE_SINK_H */

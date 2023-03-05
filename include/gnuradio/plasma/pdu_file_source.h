/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PDU_FILE_SOURCE_H
#define INCLUDED_PLASMA_PDU_FILE_SOURCE_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description of block+>
 * \ingroup plasma
 *
 */
class PLASMA_API pdu_file_source : virtual public gr::block
{
public:
    typedef std::shared_ptr<pdu_file_source> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::pdu_file_source.
     *
     * To avoid accidental use of raw pointers, plasma::pdu_file_source's
     * constructor is in a private implementation
     * class. plasma::pdu_file_source::make is the public interface for
     * creating new instances.
     */
    static sptr make(const std::string& data_filename,
                     const std::string& meta_filename,
                     int offset,
                     int length,
                     int pdu_length);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PDU_FILE_SOURCE_H */

/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_LFM_SOURCE_H
#define INCLUDED_PLASMA_LFM_SOURCE_H

#include <gnuradio/block.h>
#include <gnuradio/plasma/api.h>

namespace gr {
namespace plasma {

/*!
 * \brief Generates a linear frequency modulated waveform
 * \ingroup plasma
 *
 */
class PLASMA_API lfm_source : virtual public gr::block
{
public:
    typedef std::shared_ptr<lfm_source> sptr;

    /*!
     * \brief Return a shared_ptr to a new instance of plasma::lfm_source.
     *
     * To avoid accidental use of raw pointers, plasma::lfm_source's
     * constructor is in a private implementation
     * class. plasma::lfm_source::make is the public interface for
     * creating new instances.
     */
    static sptr
    make(double bandwidth, double start_freq, double pulse_width, double samp_rate);

    /**
     * @brief Set the metadata keys in the PDU output
     *
     * @param label
     * @param sample_rate
     * @param bandwidth
     * @param duration
     */
    virtual void init_meta_dict(const std::string& bandwidth_key,
                                const std::string& start_freq_key,
                                const std::string& duration_key,
                                const std::string& sample_rate_key,
                                const std::string& label_key) = 0;
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_LFM_SOURCE_H */

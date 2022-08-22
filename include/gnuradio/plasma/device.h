/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_DEVICE_H
#define INCLUDED_PLASMA_DEVICE_H

#include <gnuradio/plasma/api.h>
#include <arrayfire.h>

namespace gr {
namespace plasma {

/*!
 * \brief <+description+>
 *
 */
class PLASMA_API Device
{
public:
    enum Backend { DEFAULT, CPU, CUDA, OPENCL };
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_DEVICE_H */

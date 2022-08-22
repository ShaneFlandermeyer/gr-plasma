/* -*- c++ -*- */
/*
 * Copyright 2022 gr-plasma author.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_PLASMA_PHASE_CODE_H
#define INCLUDED_PLASMA_PHASE_CODE_H

#include <gnuradio/plasma/api.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace gr {
namespace plasma {

/*!
 * \brief <+description+>
 *
 */
class PLASMA_API PhaseCode
{
public:
  /**
   * Enumeration of supported phase code types
   *
   */
  enum Code {
    /**
     * Barker code
     *
     */
    BARKER,
    /**
     * Frank code
     *
     */
    FRANK,
    /**
     * P4 Code
     * 
     */
    P4,
    /**
     * Arbitrary (currently invalid) code type
     *
     */
    GENERIC = 999
  };

  /**
   * @brief Return the phase values of a phase code of length n
   *
   * @param type Code type
   * @param n Code length
   * @return std::vector<double> Phase code vector
   */
  static std::vector<double> generate_code(Code type, int n);

  static std::string code_string(Code);

  static void wrapToPi(std::vector<double>&);
};

} // namespace plasma
} // namespace gr

#endif /* INCLUDED_PLASMA_PHASE_CODE_H */

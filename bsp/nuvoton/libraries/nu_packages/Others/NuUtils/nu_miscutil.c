/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nu_miscutil.h"
#include <math.h>

/**
 * @brief Convert nanoseconds to CPU cycles (ceiling).
 *
 * Converts a time duration in nanoseconds to the equivalent number of CPU cycles
 * at a given clock frequency, rounding up to the nearest integer.
 *
 * @param ns[in]      Time duration in nanoseconds
 * @param clk_hz[in]  CPU clock frequency in Hz
 *
 * @return Number of CPU cycles (rounded up)
 *
 * @note Returns 0 if ns or clk_hz is <= 0
 */
uint32_t ns_to_cycles_ceil(double ns, double clk_hz)
{
    /* Validate input parameters */
    if (ns <= 0.0 || clk_hz <= 0.0)
        return 0;

    /* Calculate cycles: ceil(ns * clk_hz / 1e9) */
    double cycles = (ns * clk_hz) / 1e9;
    return (uint32_t)ceil(cycles);
}

/**
 * @brief Convert frequency in Hz to time period in nanoseconds (ceiling).
 *
 * Converts a frequency in Hz to the equivalent time period in nanoseconds,
 * rounding up to the nearest integer.
 *
 * @param hz[in]  Frequency in Hz
 *
 * @return Time period in nanoseconds (rounded up)
 *
 * @note Returns 0 if hz is <= 0
 */
uint32_t hz_to_ns_ceil(double hz)
{
    /* Validate input parameter */
    if (hz <= 0.0)
        return 0;

    /* Calculate nanoseconds: ceil(1e9 / hz) */
    double ns = 1e9 / hz;
    return (uint32_t)ceil(ns);
}

/**
 * @brief Convert nanoseconds to CPU cycles (floating point).
 *
 * Converts a time duration in nanoseconds to the equivalent number of CPU cycles
 * at a given clock frequency, returning a floating-point result.
 *
 * @param ns[in]      Time duration in nanoseconds
 * @param clk_hz[in]  CPU clock frequency in Hz
 *
 * @return Number of CPU cycles as a floating-point value
 *
 * @note Returns 0 if ns or clk_hz is <= 0
 */
double ns_to_cycles(double ns, double clk_hz)
{
    /* Validate input parameters */
    if (ns <= 0.0 || clk_hz <= 0.0)
        return 0;

    /* Calculate cycles: ns * clk_hz / 1e9 */
    return (ns * clk_hz) / 1e9;
}

/**
 * @brief Convert frequency in Hz to time period in nanoseconds (floating point).
 *
 * Converts a frequency in Hz to the equivalent time period in nanoseconds,
 * returning a floating-point result.
 *
 * @param hz[in]  Frequency in Hz
 *
 * @return Time period in nanoseconds as a floating-point value
 *
 * @note Returns 0 if hz is <= 0
 */
double hz_to_ns(double hz)
{
    /* Validate input parameter */
    if (hz <= 0.0)
        return 0;

    /* Calculate nanoseconds: 1e9 / hz */
    return 1e9 / hz;
}

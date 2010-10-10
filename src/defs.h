
/**
 * @file defs.h
 * @brief common definitions
 */

#pragma once

/**
 * @warning the value of @a avg will be modified
 *
 * @param[in,out] avg the running avg (reference will be modified)
 * @param[in] new_value the new value to mix in
 * @param[in] n number of significant generations for tuning
 */
template <typename T>
inline void expMovAvg (T& avg, T new_value, double n)
{
    Q_ASSERT(n >= 1);
    qreal alpha = 2.0 / (n + 1.0);       // the alpha smoothing factor
    avg = (new_value * alpha) + (avg * (1.0 - alpha));
}


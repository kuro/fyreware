/*
 * Copyright 2010 Blanton Black
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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


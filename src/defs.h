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

#include <math.h>
#include <QDebug>
#include <LinearMath/btVector3.h>

/**
 * @warning the value of @a avg will be modified
 *
 * @param[in,out] avg the running avg (reference will be modified)
 * @param[in] new_value the new value to mix in
 * @param[in] n number of significant generations for tuning
 */
template <typename T>
inline void expMovAvg (T& avg, T new_value, qreal n)
{
    Q_ASSERT(n >= 1.0);
    qreal alpha = 2.0 / (n + 1.0);       // the alpha smoothing factor
    avg = (new_value * alpha) + (avg * (1.0 - alpha));
}

const qreal        pi = 4.0 * atan(1.0);
const qreal   half_pi = pi * 0.5;
const qreal quater_pi = pi * 0.25;

inline
qreal randf (qreal max = 1.0)
{
    return (qreal(qrand()) / RAND_MAX) * max;
}

inline
qreal randf (qreal min, qreal max)
{
    qreal diff (max - min);
    return (qreal(qrand()) / RAND_MAX) * diff + min;
}

inline
QDebug operator<< (QDebug& d, const btVector3& v)
{
    QString str ("btVector3(%0, %1, %2)");
    str = str.arg(v.x());
    str = str.arg(v.y());
    str = str.arg(v.z());
    d << str;
    return d;
}

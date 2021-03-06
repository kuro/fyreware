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
 * @file Camera.cpp
 * @brief Camera implementation
 */

#include "Camera.moc"

#include "defs.h"

#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btMatrix3x3.h>
#include <LinearMath/btTransform.h>

#include <QDebug>

#include <qgl.h>

#include <math.h>

struct Camera::Private
{
    btVector3 position;
    btQuaternion orientation;

    btVector3 smoothedPosition;
    btQuaternion smoothedOrientation;

    btVector3 prevPosition;

    Private (Camera* q) :
        position(0.0, 0.0, 0.0),
        orientation(btQuaternion::getIdentity()),
        smoothedPosition(position),
        smoothedOrientation(orientation),
        prevPosition(position)
    {
        Q_UNUSED(q);
    }
};

Camera::Camera (QObject* parent) :
    QObject(parent),
    d(new Private(this))
{
}

Camera::~Camera ()
{
}

btQuaternion Camera::orientation () const
{
    return d->orientation;
}

btVector3 Camera::position () const
{
    return d->position;
}

void Camera::setPosition (const btVector3& position)
{
    d->prevPosition = d->position;
    d->position = position;
}

btVector3 Camera::forward () const
{
    return quatRotate(d->orientation, btVector3(0, 0, -1));
}

btVector3 Camera::right () const
{
    return quatRotate(d->orientation, btVector3(0, 1, 0));
}

btVector3 Camera::up () const
{
    return quatRotate(d->orientation, btVector3(1, 0, 0));
}

btVector3 Camera::velocity () const
{
    return d->position - d->prevPosition;
}

void Camera::lookAt (const btVector3& p)
{
    btVector3 x, y, z;
    y = btVector3(0, 1, 0);
    z = (d->position - p).normalize();
    x = (y.cross(z)).normalize();
    y = z.cross(x);
    btMatrix3x3 mat (
        x[0], x[1], x[2],
        y[0], y[1], y[2],
        z[0], z[1], z[2]
        );
    mat = mat.inverse();
    mat.getRotation(d->orientation);
}

void Camera::invoke ()
{
    expMovAvg(d->smoothedPosition   , d->position   , 8);
    expMovAvg(d->smoothedOrientation, d->orientation, 8);

    btTransform xform (d->smoothedOrientation, d->smoothedPosition);
    xform = xform.inverse();
    float m[16];
    xform.getOpenGLMatrix(m);
    glMultMatrixf(m);
}

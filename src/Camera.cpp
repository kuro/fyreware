
/**
 * @file Camera.cpp
 * @brief Camera implementation
 */

#include "Camera.moc"

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

    Private (Camera* q)
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

btVector3 Camera::position () const
{
    return d->position;
}

void Camera::setPosition (const btVector3& position)
{
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

void Camera::invoke () const
{
    btTransform xform (d->orientation, d->position);
    xform = xform.inverse();
    float m[16];
    xform.getOpenGLMatrix(m);
    glMultMatrixf(m);
}

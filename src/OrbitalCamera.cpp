
/**
 * @file OrbitalCamera.cpp
 * @brief OrbitalCamera implementation
 */

#include "OrbitalCamera.moc"

#include "defs.h"

#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>

struct OrbitalCamera::Private
{
    qreal distance;
    qreal altitude;
    qreal azimuth;
    btVector3 focus;
    qreal maxDistance;

    Private (OrbitalCamera* q) :
        distance(200),
        altitude(half_pi),
        azimuth(-half_pi),
        focus(0.0, 0.0, 0.0),
        maxDistance(0.0)
    {
        Q_UNUSED(q);
    }
};

OrbitalCamera::OrbitalCamera (QObject* parent) :
    Camera(parent),
    d(new Private(this))
{
}

OrbitalCamera::~OrbitalCamera ()
{
}

void OrbitalCamera::setMaxDistance (qreal maxDistance)
{
    d->maxDistance = maxDistance;
}

qreal OrbitalCamera::distance () const
{
    return d->distance;
}

qreal OrbitalCamera::altitude () const
{
    return d->altitude;
}

qreal OrbitalCamera::azimuth () const
{
    return d->azimuth;
}

qreal& OrbitalCamera::rdistance ()
{
    return d->distance;
}

qreal& OrbitalCamera::raltitude ()
{
    return d->altitude;
}

qreal& OrbitalCamera::razimuth ()
{
    return d->azimuth;
}

void OrbitalCamera::setDistance (qreal distance)
{
    d->distance = qMax(1.0, distance);
    if (qFuzzyCompare(d->maxDistance, 0.0)) {
        d->distance = qMin(distance, d->maxDistance);
    }
}

void OrbitalCamera::setAltitude (qreal altitude)
{
    d->altitude = qBound(0.000001, altitude, pi - 0.000001f);
}

void OrbitalCamera::setAzimuth (qreal azimuth)
{
    d->azimuth = azimuth;

}

void OrbitalCamera::setFocus (const btVector3& focus)
{
    d->focus = focus;
}

void OrbitalCamera::invoke ()
{
    setPosition(btVector3(
            d->focus.x() + d->distance * sin(d->altitude) * cos(d->azimuth),
            d->focus.y() + d->distance * cos(d->altitude),
            d->focus.z() + d->distance * sin(d->altitude) * sin(d->azimuth)
            ));
    lookAt(d->focus);

    Camera::invoke();

    //qDebug("distance=%f altitude=%f azimuth=%f",
           //d->distance, d->altitude, d->azimuth);

    //qDebug("position %f %f %f", position().x(), position().y(), position().z());
    //qDebug("orientation %f %f %f %f", orientation().w(), orientation().x(), orientation().y(), orientation().z());
}

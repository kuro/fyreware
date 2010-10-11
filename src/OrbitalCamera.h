
/**
 * @file OrbitalCamera.h
 * @brief OrbitalCamera definition
 */

#pragma once

#include "Camera.h"

class OrbitalCamera : public Camera
{
    Q_OBJECT

public:
    OrbitalCamera (QObject* parent = NULL);
    virtual ~OrbitalCamera ();

    void setMaxDistance (qreal maxDistance);

    qreal distance () const;
    qreal altitude () const;
    qreal azimuth () const;

    void setDistance (qreal distance);
    void setAltitude (qreal altitude);
    void setAzimuth (qreal azimuth);

    void setFocus (const btVector3& focus);

    virtual void invoke ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

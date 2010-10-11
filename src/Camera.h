
/**
 * @file Camera.h
 * @brief Camera definition
 */

#pragma once

#include <QObject>

struct btVector3;

class Camera : public QObject
{
    Q_OBJECT

public:
    Camera (QObject* parent = NULL);
    virtual ~Camera ();

    btVector3 position () const;
    void setPosition (const btVector3& position);

    btVector3 forward () const;
    btVector3 right   () const;
    btVector3 up      () const;

    void lookAt (const btVector3& p);

    void invoke () const;

private:
    struct Private;
    QScopedPointer<Private> d;
};

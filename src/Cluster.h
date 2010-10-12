
/**
 * @file Cluster.h
 * @brief Cluster definition
 */

#pragma once

#include <QObject>

class btVector3;

class Cluster : public QObject
{
    Q_OBJECT

public:
    Cluster (const btVector3& origin, QObject* parent = NULL);
    virtual ~Cluster ();

    Q_INVOKABLE void emitStar (qreal vx, qreal vy, qreal vz);

public slots:
    void draw ();
    void update (qreal dt);

private:
    void setup ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

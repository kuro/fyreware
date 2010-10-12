
/**
 * @file Cluster.cpp
 * @brief Cluster implementation
 */

#include "Cluster.moc"

#include "defs.h"

#include <LinearMath/btVector3.h>

#include <QVector>
#include <QGLWidget>
#include <QDebug>

struct Cluster::Private
{
    btVector3 origin;
    QVector<btVector3> initialVelocities;
    QVector<btVector3> origins;
    qreal lifetime;
    qreal age;
    QHash<QString, btVector3> colorTable;
    btVector3 color;

    Private (const btVector3& origin, Cluster* q) :
        origin(origin),
        lifetime(10),
        age(0.0)
    {
        Q_UNUSED(q);
    }
};

Cluster::Cluster (const btVector3& origin, QObject* parent) :
    QObject(parent),
    d(new Private(origin, this))
{
    setup();

    // populate array of initial velocities
    d->origins.fill(d->origin, d->initialVelocities.size());

    // color
    d->colorTable.insert("red"           , btVector3(1.0 , 0.0 , 0.0 ));
    d->colorTable.insert("orange"        , btVector3(1.0 , 0.6 , 0.0 ));
    d->colorTable.insert("gold"          , btVector3(1.0 , 0.84, 0.0 ));
    d->colorTable.insert("yellow"        , btVector3(1.0 , 1.0 , 0.0 ));
    d->colorTable.insert("green"         , btVector3(0.0 , 1.0 , 0.0 ));
    d->colorTable.insert("blue"          , btVector3(0.0 , 0.0 , 1.0 ));
    d->colorTable.insert("purple"        , btVector3(0.50, 0.50, 0.0 ));
    d->colorTable.insert("silver"        , btVector3(0.75, 0.75, 0.75));

    QList<btVector3> colors (d->colorTable.values());
    d->color = colors[floor(randf(colors.size()))];
}

Cluster::~Cluster ()
{
}

void Cluster::setup ()
{
    for (int i = 0; i < 1024; i++) {
        btVector3 direction (
            randf(-10, 10),
            randf(-10, 10),
            randf(-10, 10)
            );
        direction = direction.normalize();
        direction *= 10;
        emitStar(direction.x(), direction.y(), direction.z());
    }
}

void Cluster::emitStar (qreal vx, qreal vy, qreal vz)
{
    btVector3 initialVelocity (vx, vy, vz);
    d->initialVelocities << initialVelocity;
}

void Cluster::update (qreal dt)
{
    if (d->age >= d->lifetime) {
        deleteLater();
    } else {
        d->age += dt;
    }
}

void Cluster::draw ()
{
    btVector3 g (0, -9.806, 0);

    glColor3fv(d->color);

    glPointSize(10);
    glBegin(GL_POINTS);
    foreach (const btVector3& v0, d->initialVelocities) {
        qreal& t = d->age;
        btVector3 p = d->origin + v0 * t + g * 0.5 * t * t;
        glVertex3f(p.x(), p.y(), p.z());
    }
    glEnd();
}

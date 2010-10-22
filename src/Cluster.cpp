
/**
 * @file Cluster.cpp
 * @brief Cluster implementation
 */

#include "Cluster.moc"

#include "defs.h"
#include "Scene.h"
#include "ShaderProgram.h"
#include "Camera.h"

#include <LinearMath/btVector3.h>

#include <QVector>
#include <QGLWidget>
#include <QDebug>

#include <Cg/cgGL.h>

struct Cluster::Private
{
    btVector3 origin;
    QVector<btVector3> initialVelocities;
    qreal lifetime;
    qreal age;
    QHash<QString, btVector3> colorTable;
    btVector3 color;
    int starCount;

    struct {
        CGparameter v0;
        CGparameter t;
        CGparameter nt;
        CGparameter origin;
        CGparameter eye;
    } shader;

    Private (const btVector3& origin, Cluster* q) :
        origin(origin),
        lifetime(4),
        age(0.0),
        starCount(0)
    {
        Q_UNUSED(q);
    }
};

Cluster::Cluster (const btVector3& origin, QObject* parent) :
    QObject(parent),
    d(new Private(origin, this))
{
    setup();

    CGprogram prog = scene->shader("fyreworks")->program();
    d->shader.v0     = cgGetNamedParameter(prog, "v0");
    d->shader.t      = cgGetNamedParameter(prog, "t");
    d->shader.nt     = cgGetNamedParameter(prog, "nt");
    d->shader.origin = cgGetNamedParameter(prog, "origin");
    d->shader.eye    = cgGetNamedParameter(prog, "eye");

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
    d->starCount++;
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
    glColor3fv(d->color);

    cgGLEnableClientState(d->shader.v0);
    cgGLSetParameter1f(d->shader.t, d->age);
    cgGLSetParameter1f(d->shader.nt, d->age/d->lifetime);
    cgGLSetParameterPointer(d->shader.v0, 3, GL_FLOAT, sizeof(btVector3),
                            d->initialVelocities[0]);
    cgGLSetParameter3fv(d->shader.origin, d->origin);
    cgGLSetParameter3fv(d->shader.eye, scene->camera()->position());
    glDrawArrays(GL_POINTS, 0, d->starCount);
    cgGLDisableClientState(d->shader.v0);
}

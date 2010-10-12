
/**
 * @file Shell.cpp
 * @brief Shell implementation
 */

#include "Shell.moc"

#include "Scene.h"
#include "defs.h"

#include <btBulletDynamicsCommon.h>

#include <QDebug>
#include <QGLWidget>

struct Shell::Private
{
    qreal radius;
    btCollisionShape* shape;  ///< @todo Individual shapes is unnecessary.
    btRigidBody* rigidBody;
    btTransform trx;
    qreal lifetime;
    qreal age;

    Private (Shell* q) :
        radius(1.0),  // 1 meter is really huge, but visible
        shape(NULL),
        rigidBody(NULL),
        trx(btQuaternion::getIdentity(), btVector3(0, 0, 0)),
        lifetime(1.0),
        age(0)
    {
        Q_UNUSED(q);
    }
};

Shell::Shell (QObject* parent) :
    QObject(parent),
    d(new Private(this))
{
    btScalar mass = 1.0;

    btVector3 inertia (0, 0, 0);
    d->shape = new btSphereShape(d->radius);
    Q_CHECK_PTR(d->shape);

    d->shape->calculateLocalInertia(mass, inertia);

    btRigidBody::btRigidBodyConstructionInfo conInfo (
        mass, this, d->shape, inertia);

    d->rigidBody = new btRigidBody(conInfo);
    Q_CHECK_PTR(d->rigidBody);

    scene->dynamicsWorld()->addRigidBody(d->rigidBody);

    // shooting for a height of about 100 meters
    d->rigidBody->applyCentralImpulse(btVector3(
            randf(-10, 10),
            randf(60, 80),
            randf(-10, 10)));

    d->lifetime = randf(1.5, 2.0);

    //qDebug() << "v" << d->rigidBody->getLinearVelocity().length();
}

Shell::~Shell ()
{
    delete d->rigidBody;
    delete d->shape;
}

void Shell::getWorldTransform (btTransform& trx) const
{
    trx = d->trx;
}

void Shell::setWorldTransform (const btTransform& trx)
{
    d->trx = trx;
}

void Shell::draw ()
{
    glPushMatrix();

    float m[16];
    d->trx.getOpenGLMatrix(m);
    glMultMatrixf(m);

    static GLuint dlist = 0;
    if (glIsList(dlist)) {
        glCallList(dlist);
    } else {
        dlist = glGenLists(1);
        glNewList(dlist, GL_COMPILE_AND_EXECUTE);

        GLUquadric* quadric = gluNewQuadric();
        gluSphere(quadric, d->radius, 3, 3);
        gluDeleteQuadric(quadric);

        glEndList();
    }

    glPopMatrix();
}

void Shell::update (qreal dt)
{
    if (d->age >= d->lifetime) {
        /// @todo explode
        scene->dynamicsWorld()->removeRigidBody(d->rigidBody);
        deleteLater();
    } else {
        d->age += dt;
    }
}

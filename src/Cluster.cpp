
/**
 * @file Cluster.cpp
 * @brief Cluster implementation
 */

#include "Cluster.moc"

#include "defs.h"
#include "scripting.h"
#include "Scene.h"
#include "ShaderProgram.h"
#include "Camera.h"
#include "SoundEngine.h"

#include <QVector>
#include <QGLWidget>
#include <QDebug>
#include <QScriptEngine>

#include <Cg/cgGL.h>

#include <QtFMOD/System.h>
#include <QtFMOD/Channel.h>
#include <QtFMOD/Sound.h>

struct Cluster::Private
{
    btVector3 origin;
    QVector<btVector3> initialVelocities;
    qreal lifetime;
    qreal age;
    QHash<QString, btVector3> colorTable;
    btVector3 color;
    int starCount;

    QSharedPointer<QtFMOD::Channel> channel;

    QScriptProgram& shellProgram;

    struct {
        CGparameter v0;
        CGparameter t;
        CGparameter nt;
        CGparameter origin;
        CGparameter eye;
    } shader;

    Private (const btVector3& origin, QScriptProgram& shellProgram,
             Cluster* q) :
        origin(origin),
        lifetime(4),
        age(0.0),
        starCount(0),
        shellProgram(shellProgram)
    {
        Q_UNUSED(q);
    }
};

Cluster::Cluster (const btVector3& origin, QScriptProgram& shellProgram,
                  QObject* parent) :
    QObject(parent),
    d(new Private(origin, shellProgram, this))
{
    setup();

    CGprogram prog = scene->shader("fyreworks")->program();
    d->shader.v0     = cgGetNamedParameter(prog, "v0");
    d->shader.t      = cgGetNamedParameter(prog, "t");
    d->shader.nt     = cgGetNamedParameter(prog, "nt");
    d->shader.origin = cgGetNamedParameter(prog, "origin");
    d->shader.eye    = cgGetNamedParameter(prog, "eye");

    // color
    d->colorTable.insert("red"           , btVector3(1.0f , 0.0f , 0.0f ));
    d->colorTable.insert("orange"        , btVector3(1.0f , 0.6f , 0.0f ));
    d->colorTable.insert("gold"          , btVector3(1.0f , 0.84f, 0.0f ));
    d->colorTable.insert("yellow"        , btVector3(1.0f , 1.0f , 0.0f ));
    d->colorTable.insert("green"         , btVector3(0.0f , 1.0f , 0.0f ));
    d->colorTable.insert("blue"          , btVector3(0.0f , 0.0f , 1.0f ));
    d->colorTable.insert("purple"        , btVector3(0.50f, 0.50f, 0.0f ));
    d->colorTable.insert("silver"        , btVector3(0.75f, 0.75f, 0.75f));

    QList<btVector3> colors (d->colorTable.values());
    d->color = colors[floor(randf(colors.size()))];

    // sound
    soundEngine->soundSystem()->playSound(
        FMOD_CHANNEL_REUSE, soundEngine->sound("explosion"), false, d->channel);
    d->channel->set3DAttributes(d->origin);
}

Cluster::~Cluster ()
{
}

/**
 * Emit script function.
 *
 * Expects a struct with the following properties:
 *   - direction
 *   - speed
 *
 * The direction will be normalized automatically.
 * The speed will be used to derive the (initial) velocity.
 * This in turn will place a call to cluster->emitStar().
 */
static
QScriptValue emitFun (QScriptContext* ctx, QScriptEngine* engine)
{
    QVariant clusterVar = ctx->thisObject().property("self").toVariant();
    Cluster* cluster = clusterVar.value<Cluster*>();
    Q_ASSERT(cluster);

    QScriptValue star = ctx->argument(0);

    btVector3 dir (engine->fromScriptValue<btVector3>(
            star.property("direction")));
    qreal speed = star.property("speed").toNumber();

    btVector3 vel = dir.normalized() * speed;
    cluster->emitStar(vel);

    return QScriptValue();
}

void Cluster::setup ()
{
    QScriptEngine* engine = scene->scriptEngine();
    QScriptContext* ctx = engine->pushContext();
    QScriptValue ao = ctx->activationObject();
    prepGlobalObject(ao);
    ao.setProperty("emit", engine->newFunction(emitFun));

    /// @todo is this the best way to get access to the cluster?
    QVariant var = qVariantFromValue(this);
    ao.setProperty("self", engine->newVariant(var));

    engine->evaluate(d->shellProgram);
    engine->popContext();
}

void Cluster::emitStar (btVector3 initialVelocity)
{
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

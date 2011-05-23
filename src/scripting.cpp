
/**
 * @file scripting.cpp
 * @brief scripting implementation
 */

#include "scripting.h"

#include "defs.h"
#include "Scene.h"

#include <QScriptEngine>

static
QScriptValue randFun (QScriptContext* ctx, QScriptEngine* eng)
{
    switch (ctx->argumentCount()) {
    case 1:
        return randf(ctx->argument(0).toNumber());
    case 2:
        return randf(ctx->argument(0).toNumber(),
                     ctx->argument(1).toNumber());
    case 0:
    default:
        return randf();
    }
}

QScriptValue btVector3ToScriptValue (QScriptEngine* engine, const btVector3& v)
{
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", v.x());
    obj.setProperty("y", v.y());
    obj.setProperty("z", v.z());
    return obj;
}

void scriptValue2btVector3 (const QScriptValue& obj, btVector3& v)
{
    v.setX(obj.property("x").toNumber());
    v.setY(obj.property("y").toNumber());
    v.setZ(obj.property("z").toNumber());
}

QScriptValue btVector3Ctor (QScriptContext* ctx, QScriptEngine* engine)
 {
     btVector3 v (0, 0, 0);
     switch (ctx->argumentCount()) {
     case 3:
         v.setZ(ctx->argument(2).toNumber());
     case 2:
         v.setY(ctx->argument(1).toNumber());
     case 1:
         v.setX(ctx->argument(0).toNumber());
     }
     return engine->toScriptValue(v);
 }

static
QScriptValue crossFun (QScriptContext* ctx, QScriptEngine* eng)
{
    if (ctx->argumentCount() != 2) {
        qWarning() << Q_FUNC_INFO << "invalid argument count";
        return QScriptValue();
    }

    btVector3 a (eng->fromScriptValue<btVector3>(ctx->argument(0)));
    btVector3 b (eng->fromScriptValue<btVector3>(ctx->argument(1)));
    btVector3 r (a.cross(b));

    return eng->toScriptValue(r);
}

static
QScriptValue addFun (QScriptContext* ctx, QScriptEngine* eng)
{
    if (ctx->argumentCount() != 2) {
        qWarning() << Q_FUNC_INFO << "invalid argument count";
        return QScriptValue();
    }

    btVector3 a (eng->fromScriptValue<btVector3>(ctx->argument(0)));
    btVector3 b (eng->fromScriptValue<btVector3>(ctx->argument(1)));
    btVector3 r (a + b);

    return eng->toScriptValue(r);
}


void prepGlobalObject (QScriptValue& sv)
{
    QScriptEngine* engine = scene->scriptEngine();

    sv.setProperty("rand" , engine->newFunction(randFun ));
    sv.setProperty("cross", engine->newFunction(crossFun));
    sv.setProperty("add", engine->newFunction(addFun));

    // spectrum
    QScriptValue spectrumSv  = engine->newArray(2);
    QScriptValue spectrum0Sv = engine->newArray(scene->spectrum()[0].size());
    QScriptValue spectrum1Sv = engine->newArray(scene->spectrum()[1].size());
    spectrumSv.setProperty(0, spectrum0Sv);
    spectrumSv.setProperty(1, spectrum1Sv);

    for (int i = 0; i < scene->spectrum()[0].size(); i++) {
        spectrum0Sv.setProperty(i, scene->spectrum()[0][i]);
        spectrum1Sv.setProperty(i, scene->spectrum()[1][i]);
    }
    sv.setProperty("spectrum", spectrumSv);

    // types
    qScriptRegisterMetaType(engine, btVector3ToScriptValue,
                            scriptValue2btVector3);
    sv.setProperty("vec3", engine->newFunction(btVector3Ctor));
}

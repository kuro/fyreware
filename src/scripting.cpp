
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

static
QScriptValue launchFun (QScriptContext* ctx, QScriptEngine* eng)
{
    scene->launch();
    return QScriptValue();
}

void initScripting (QScriptEngine* engine)
{
    engine->globalObject().setProperty(
        "rand", engine->newFunction(randFun));
    engine->globalObject().setProperty(
        "launch", engine->newFunction(launchFun));
}


/**
 * @file ShaderProgram.cpp
 * @brief ShaderProgram implementation
 */

#include "ShaderProgram.moc"

#include "Shader.h"

struct ShaderProgram::Private
{
    CGcontext context;
    CGprofile fragmentProfile;
    CGprogram program;

    QList<Shader*> shaders;

    Private (ShaderProgram* q)
    {
        Q_UNUSED(q);
    }
};

ShaderProgram::ShaderProgram (QObject* parent) :
    QObject(parent),
    d(new Private(this))
{
    d->context = cgCreateContext();
}

ShaderProgram::~ShaderProgram ()
{
}

CGerror ShaderProgram::error () const
{
    return cgGetError();
}

QString ShaderProgram::errorString () const
{
    CGerror err = cgGetError();
    return cgGetErrorString(err);
}

bool ShaderProgram::addShaderFromSourceCode (
    CGGLenum profileClass, const QString& code, const QString& entryPoint)
{
    Shader* shader = new Shader(profileClass, this);
    shader->compileSourceCode(d->context, code, entryPoint);
    d->shaders << shader;
    return true;
}

bool ShaderProgram::addShaderFromSourceFile (
    CGGLenum profileClass, const QString& file, const QString& entryPoint)
{
    Shader* shader = new Shader(profileClass, this);
    shader->compileSourceFile(d->context, file, entryPoint);
    d->shaders << shader;
    return true;
}

bool ShaderProgram::link ()
{
    switch (d->shaders.size()) {
    case 1:
        d->program = d->shaders[0]->program();
        break;
    case 2:
        d->program = cgCombinePrograms2(d->shaders[0]->program(),
                                        d->shaders[1]->program());
        cgDestroyProgram(d->shaders[0]->program());
        cgDestroyProgram(d->shaders[1]->program());
        break;
    default:
        break;
    }
    cgGLLoadProgram(d->program);
    return true;
}

bool ShaderProgram::bind ()
{
    foreach (const Shader* shader, d->shaders) {
        cgGLEnableProfile(shader->profile());
    }
    cgGLBindProgram(d->program);
    return true;
}

void ShaderProgram::release ()
{
    foreach (const Shader* shader, d->shaders) {
        cgGLDisableProfile(shader->profile());
    }
}

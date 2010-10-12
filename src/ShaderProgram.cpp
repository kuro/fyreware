/*
 * Copyright 2010 Blanton Black
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

    CGerror error;

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

CGprogram ShaderProgram::program () const
{
    return d->program;
}

/**
 * @warning Calling this consumes an error message.
 */
CGerror ShaderProgram::error () const
{
    d->error = cgGetError();
    return d->error;
}

/**
 * Must call error() (once) first.
 */
QString ShaderProgram::errorString () const
{
    return cgGetErrorString(d->error);
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

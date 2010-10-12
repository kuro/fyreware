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
 * @file Shader.cpp
 * @brief Shader implementation
 */

#include "Shader.moc"

#include <QFile>

struct Shader::Private
{
    CGGLenum profileClass;

    CGprofile profile;
    CGprogram program;

    Private (CGGLenum profileClass, Shader* q) :
        profileClass(profileClass)
    {
        Q_UNUSED(q);
    }
};

Shader::Shader (CGGLenum profileClass, QObject* parent) :
    QObject(parent),
    d(new Private(profileClass, this))
{
}

Shader::~Shader ()
{
}

bool Shader::compileSourceCode (CGcontext context,
                                const QString& code, const QString& entryPoint)
{
    d->profile = cgGLGetLatestProfile(d->profileClass);

    qDebug("profile: %s", cgGetProfileString(d->profile));

    cgGLSetOptimalOptions(d->profile);
    d->program = cgCreateProgram(context, CG_SOURCE, code.toLocal8Bit(),
                                 d->profile, entryPoint.toLocal8Bit(), NULL);
    return true;
}

bool Shader::compileSourceFile (CGcontext context,
                                const QString& file, const QString& entryPoint)
{
    QFile dev (file);
    if (!dev.open(QIODevice::ReadOnly)) {
        qWarning("failed to open file: %s", qPrintable(dev.errorString()));
        return false;
    }
    return compileSourceCode(context, dev.readAll(), entryPoint);
}

CGprofile Shader::profile () const
{
    return d->profile;
}

CGprogram Shader::program () const
{
    return d->program;
}

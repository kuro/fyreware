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
 * @file Shader.h
 * @brief Shader definition
 */

#pragma once

#include <QObject>

#include <Cg/cgGL.h>

class Shader : public QObject
{
    Q_OBJECT

public:
    Shader (CGGLenum profileClass, QObject* parent = NULL);
    virtual ~Shader ();

    CGprofile profile () const;
    CGprogram program () const;

    bool compileSourceCode (CGcontext context,
                            const QString& code, const QString& entryPoint);
    bool compileSourceFile (CGcontext context,
                            const QString& file, const QString& entryPoint);

private:
    struct Private;
    QScopedPointer<Private> d;
};

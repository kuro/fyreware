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
 * @file ShaderProgram.h
 * @brief ShaderProgram definition
 */

#pragma once

#include <QObject>

#include <Cg/cgGL.h>

class ShaderProgram : public QObject
{
    Q_OBJECT

public:
    ShaderProgram (QObject* parent = NULL);
    virtual ~ShaderProgram ();

    bool isNull() const;

    bool addShaderFromSourceCode (CGGLenum profileClass, const QString& code,
                                  const QString& entryPoint);
    bool addShaderFromSourceFile (CGGLenum profileClass, const QString& file,
                                  const QString& entryPoint);

    bool link ();
    bool bind ();
    void release ();

    CGerror error () const;
    QString errorString () const;

    CGprogram program () const;

private:
    struct Private;
    QScopedPointer<Private> d;
};

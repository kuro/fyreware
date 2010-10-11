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
 * @file Camera.h
 * @brief Camera definition
 */

#pragma once

#include <QObject>

struct btVector3;
struct btQuaternion;

class Camera : public QObject
{
    Q_OBJECT

public:
    Camera (QObject* parent = NULL);
    virtual ~Camera ();

    btQuaternion orientation () const;

    btVector3 position () const;
    void setPosition (const btVector3& position);

    btVector3 forward () const;
    btVector3 right   () const;
    btVector3 up      () const;

    void lookAt (const btVector3& p);

    virtual void invoke ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

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
 * @file Scene.h
 * @brief Scene definition
 */

#pragma once

#include <QGLWidget>
#include <QPointer>

#include "Scene.h"

#include <btBulletDynamicsCommon.h>

class QDir;
class QGestureEvent;
class QPinchGesture;
class QSwipeGesture;

class Camera;
class ShaderProgram;

class Scene : public QGLWidget
{
    Q_OBJECT

public:
    Scene (QWidget* parent = NULL);
    virtual ~Scene ();

    btDynamicsWorld* dynamicsWorld () const;

    ShaderProgram* shader (const QString& name) const;
    Camera* camera () const;

signals:
    void drawShells ();
    void drawClusters ();
    void update (qreal dt);

private:
    void loadSong (const QString& fileName);
    void loadCubeMap (const QDir& path);
    void makeStarTex (int maxWidth);

    void initPhysics ();

    void initializeGL ();
    void resizeGL (int w, int h);
    void paintGL ();

    void drawSceneShells ();
    void drawSceneClusters ();
    void drawSpectrum ();
    void drawSky ();

    void showEvent (QShowEvent* evt);
    void closeEvent (QCloseEvent* evt);

    void wheelEvent (QWheelEvent* evt);

    bool event (QEvent* evt);
    bool gestureEvent (QGestureEvent* evt);
    void pinchGesture (QPinchGesture* gesture);
    void swipeGesture (QSwipeGesture* gesture);

    void checkTags ();
    void analyzeSound ();

    void launch ();

    static void internalTickCallback (
        btDynamicsWorld* world, btScalar timeStep);

private slots:
    void on_timer_timeout ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

extern QPointer<Scene> scene;

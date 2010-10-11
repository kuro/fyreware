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

class QDir;

class Scene : public QGLWidget
{
    Q_OBJECT

public:
    Scene (QWidget* parent = NULL);
    virtual ~Scene ();

private:
    void loadSong (const QString& fileName);
    void loadCubeMap (const QDir& path);

    void initializeGL ();
    void resizeGL (int w, int h);
    void paintGL ();

    void drawSpectrum ();
    void drawSky ();

    void showEvent (QShowEvent* evt);
    void closeEvent (QCloseEvent* evt);

private slots:
    void on_timer_timeout ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

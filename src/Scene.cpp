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
 * @file Scene.cpp
 * @brief Scene implementation
 */

#include "Scene.moc"

#include "defs.h"

#include <QTimer>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QIcon>

#include <QtFMOD/System.h>
#include <QtFMOD/Channel.h>
#include <QtFMOD/Sound.h>

#define SPECTRUM_HEIGHT 2
#define SPECTRUM_GENERATIONS 8

#define fsysCheck()                                                         \
    do {                                                                    \
        if (d->fsys->error() != FMOD_OK) {                                  \
            qWarning() << d->fsys->errorString();                           \
        }                                                                   \
    } while (0)

#define channelCheck()                                                      \
    do {                                                                    \
        if (d->channel->error() != FMOD_OK) {                               \
            qWarning() << d->channel->errorString();                        \
        }                                                                   \
    } while (0)

#define soundCheck()                                                        \
    do {                                                                    \
        if (d->sound->error() != FMOD_OK) {                                 \
            qWarning() << d->sound->errorString();                          \
        }                                                                   \
    } while (0)

struct Scene::Private
{
    QSettings* settings;

    QTimer* timer;
    QtFMOD::System* fsys;

    QSharedPointer<QtFMOD::Channel> channel;
    QSharedPointer<QtFMOD::Sound> sound;

    int spectrumLength;
    FMOD_DSP_FFT_WINDOW spectrumWindowType;
    QVector<float> spectrumNew[2];      ///< values before smoothing
    QVector<float> spectrum[2];         ///< values after smoothing


    Private (Scene* q) :
        settings(new QSettings(q)),
        timer(new QTimer(q)),
        fsys(new QtFMOD::System(q)),
        spectrumLength(128),
        spectrumWindowType(FMOD_DSP_FFT_WINDOW_HAMMING)
    {
        timer->setObjectName("timer");
        fsys->setObjectName("fsys");

        spectrumNew[0].resize(spectrumLength);
        spectrumNew[1].resize(spectrumLength);
        spectrum[0].resize(spectrumLength);
        spectrum[1].resize(spectrumLength);

        QMetaObject::connectSlotsByName(q);
    }
};

Scene::Scene (QWidget* parent) :
    QGLWidget(parent),
    d(new Private(this))
{
    connect(d->timer, SIGNAL(timeout()), d->fsys, SLOT(update()));

    d->fsys->init(1);
    fsysCheck();

    loadSong(qApp->arguments().last());

    d->timer->start(16);
}

Scene::~Scene ()
{
}

void Scene::showEvent (QShowEvent* evt)
{
    Q_UNUSED(evt);

    resize(d->settings->value("scene/size", QSize(600, 400)).toSize());
    if (d->settings->contains("scene/pos")) {
        move(d->settings->value("scene/pos").toPoint());
    }

    if (d->settings->value("scene/isFullScreen", false).toBool()) {
        setWindowState(Qt::WindowFullScreen);
    }
}

void Scene::closeEvent (QHideEvent* evt)
{
    Q_UNUSED(evt);
    if (isFullScreen()) {
        d->settings->setValue("scene/isFullScreen", true);
    } else {
        d->settings->setValue("scene/size", size());
        d->settings->setValue("scene/pos", pos());
    }
}

void Scene::initializeGL ()
{
    qglClearColor("black");
}

void Scene::resizeGL (int w, int h)
{
    glViewport(0, 0, w, h);
}

void Scene::paintGL ()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawSpectrum();

    GLuint gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        qCritical("OpenGL Error: %s\n", gluErrorString(gl_error));
    }

}

void Scene::loadSong (const QString& fileName)
{
    d->sound = QSharedPointer<QtFMOD::Sound>(d->fsys->createStream(fileName));
    fsysCheck();
    Q_ASSERT(d->sound);

    d->fsys->playSound(FMOD_CHANNEL_REUSE, d->sound, false, d->channel);
    fsysCheck();
    Q_ASSERT(d->channel);
}

void Scene::on_timer_timeout ()
{
    int nbUpdated;
    QHash<QString, QtFMOD::Tag> tags (d->sound->tags(&nbUpdated));
    if (nbUpdated > 0) {
        if (tags.contains("TITLE")) {
            QString title (tr("%0 / %1 - FyreWare"));
            title = title.arg(tags["ARTIST"].value().toString());
            title = title.arg(tags["TITLE"].value().toString());
            setWindowTitle(title);
        }
        qDebug();
    }
    QHashIterator<QString, QtFMOD::Tag> tagIter (tags);
    while (tagIter.hasNext()) {
        tagIter.next();
        QtFMOD::Tag tag (tagIter.value());
        if (tag.updated()) {
            qDebug().nospace()
                << qPrintable(tagIter.key()) << ": "
                << qPrintable(tag.value().toString());
            if (tag.name() == "APIC") {
                setWindowIcon(QPixmap::fromImage(tag.toImage()));
            }
        }
    }

    updateGL();
}

void Scene::drawSpectrum ()
{
    if (!d->channel) {
        return;
    }
    if (!d->channel->isPlaying()) {
        return;
    }

    d->channel->spectrum(d->spectrumNew[0], 0, d->spectrumWindowType);
    d->channel->spectrum(d->spectrumNew[1], 1, d->spectrumWindowType);
    for (int i = 0; i < d->spectrumLength; i++) {
        expMovAvg(d->spectrum[0][i],d->spectrumNew[0][i],SPECTRUM_GENERATIONS);
        expMovAvg(d->spectrum[1][i],d->spectrumNew[1][i],SPECTRUM_GENERATIONS);
    }


    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, d->spectrumLength * 2, 0, SPECTRUM_HEIGHT, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    qglColor("cyan");
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < d->spectrumLength; i++) {
        glVertex2f(i, d->spectrum[0][i]);
    }
    glEnd();

    qglColor("magenta");
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < d->spectrumLength; i++) {
        /// @todo perfect horizontal alignment
        glVertex2f(d->spectrumLength * 2 - i - 1, d->spectrum[1][i]);
    }
    glEnd();

}

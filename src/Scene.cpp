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

#include "ShaderProgram.h"
#include "OrbitalCamera.h"

#include <QTimer>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QIcon>
#include <QDir>
#include <QWheelEvent>
#include <QGesture>

#include <QtFMOD/System.h>
#include <QtFMOD/Channel.h>
#include <QtFMOD/Sound.h>

#include <LinearMath/btVector3.h>

#define SPECTRUM_HEIGHT 1
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

    GLuint cubeMapTex;

    OrbitalCamera* camera;

    ShaderProgram* skyShader;

    Private (Scene* q) :
        settings(new QSettings(q)),
        timer(new QTimer(q)),
        fsys(new QtFMOD::System(q)),
        spectrumLength(256),
        spectrumWindowType(FMOD_DSP_FFT_WINDOW_RECT),
        cubeMapTex(0),
        camera(new OrbitalCamera(q)),
        skyShader(new ShaderProgram(q))
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

    d->timer->start(16);

    grabGesture(Qt::TapGesture);
    grabGesture(Qt::TapAndHoldGesture);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);
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

void Scene::closeEvent (QCloseEvent* evt)
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

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    /// @todo remove the hard coded value
    loadCubeMap(QDir("Bridge.cubemap"));

    loadSong(qApp->arguments().last());
}

void Scene::resizeGL (int w, int h)
{
    glViewport(0, 0, w, h);
}

void Scene::paintGL ()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, qreal(width())/height(), 1.0, 1000.0);
    d->camera->setMaxDistance(1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    d->camera->invoke();

    drawScene();
    drawSky();
    drawSpectrum();

    GLuint gl_error = glGetError();
    if (gl_error != GL_NO_ERROR) {
        qFatal("OpenGL Error: %s\n", gluErrorString(gl_error));
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
    if (d->sound) {
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
        expMovAvg(d->spectrum[0][i],
                  d->spectrumNew[0][i],
                  d->spectrumNew[0][i] > d->spectrum[0][i]
                  ? 1.5 : SPECTRUM_GENERATIONS);
        expMovAvg(d->spectrum[1][i],
                  d->spectrumNew[1][i],
                  d->spectrumNew[1][i] > d->spectrum[1][i]
                  ? 1.5 : SPECTRUM_GENERATIONS);
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

void Scene::loadCubeMap (const QDir& path)
{
    glGenTextures(1, &d->cubeMapTex);

    glBindTexture(GL_TEXTURE_CUBE_MAP, d->cubeMapTex);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // init shader
    d->skyShader->addShaderFromSourceFile(
        CG_GL_VERTEX, ":media/shaders/sky.cg", "main_vp");
    d->skyShader->addShaderFromSourceFile(
        CG_GL_FRAGMENT, ":media/shaders/sky.cg", "main_fp");
    d->skyShader->link();

    if (d->skyShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->skyShader->errorString();
    }

    /**
     * @bug Check this.  Using convertToGLFormat(), which flips in addition to rgb swapping.  Then, flipping x and y in the shader fragment's cube texture lookup.  And finally, swapped neg and pos y images.
     */

    // prep images
    QImage posx (path.filePath("posx.jpg"));
    QImage negx (path.filePath("negx.jpg"));
    QImage posy (path.filePath("negy.jpg"));  // swapped neg y...
    QImage negy (path.filePath("posy.jpg"));  // with the pos y here
    QImage posz (path.filePath("posz.jpg"));
    QImage negz (path.filePath("negz.jpg"));

    /// @bug verify image flipping
#if 1
    posx = convertToGLFormat(posx);
    negx = convertToGLFormat(negx);
    posy = convertToGLFormat(posy);
    negy = convertToGLFormat(negy);
    posz = convertToGLFormat(posz);
    negz = convertToGLFormat(negz);
#else
    posx = posx.rgbSwapped();
    negx = negx.rgbSwapped();
    posy = posy.rgbSwapped();
    negy = negy.rgbSwapped();
    posz = posz.rgbSwapped();
    negz = negz.rgbSwapped();
#endif

    // transfer images
    Qt::AspectRatioMode aspectRatioMode = Qt::IgnoreAspectRatio;
    Qt::TransformationMode transformMode = Qt::SmoothTransformation;
    for (int width = 2048, level = 0; width > 0; width >>= 1, level++) {
        posx = posx.scaled(width, width, aspectRatioMode, transformMode);
        negx = negx.scaled(width, width, aspectRatioMode, transformMode);
        posy = posy.scaled(width, width, aspectRatioMode, transformMode);
        negy = negy.scaled(width, width, aspectRatioMode, transformMode);
        posz = posz.scaled(width, width, aspectRatioMode, transformMode);
        negz = negz.scaled(width, width, aspectRatioMode, transformMode);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, GL_RGBA,
                     width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     posx.bits()
                    );
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, GL_RGBA,
                     width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     negx.bits()
                    );
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, GL_RGBA,
                     width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     posy.bits()
                    );
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, GL_RGBA,
                     width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     negy.bits()
                    );
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, GL_RGBA,
                     width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     posz.bits()
                    );
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, GL_RGBA,
                     width, width, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     negz.bits()
                    );
    }

}

/**
 * @todo Replace with a vertex buffer.
 */
void Scene::drawSky ()
{
    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, d->cubeMapTex);

    //glDepthMask(GL_FALSE);

    d->skyShader->bind();

    glPushMatrix();
    glTranslated(d->camera->position().x(),
                 d->camera->position().y(),
                 d->camera->position().z());
    static GLuint dlist = 0;
    if (glIsList(dlist)) {
        glCallList(dlist);
    } else {
        dlist = glGenLists(1);
        glNewList(dlist, GL_COMPILE_AND_EXECUTE);

        GLUquadric* quadric = gluNewQuadric();
        gluSphere(quadric, 10.0, 30, 30);
        gluDeleteQuadric(quadric);

        glEndList();
    }
    glPopMatrix();

    d->skyShader->release();

    if (d->skyShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->skyShader->errorString();
    }

    //glDepthMask(GL_TRUE);

    glPopAttrib();
}

void Scene::wheelEvent (QWheelEvent* evt)
{
    qreal delta;
    switch (evt->orientation()) {
    case Qt::Horizontal:
        delta = -0.001 * evt->delta();
        d->camera->setAzimuth(d->camera->azimuth() + delta);
        break;
    case Qt::Vertical:
        delta = 0.001 * evt->delta();
        d->camera->setAltitude(d->camera->altitude() + delta);
        break;
    default:
        break;
    }
}

bool Scene::event (QEvent* evt)
{
    if (evt->type() == QEvent::Gesture) {
        return gestureEvent(static_cast<QGestureEvent*>(evt));
    }
    return QGLWidget::event(evt);
}

bool Scene::gestureEvent (QGestureEvent* evt)
{
    QGesture* gesture = evt->gestures().first();
    switch (gesture->gestureType()) {
    case Qt::TapGesture:
        break;
    case Qt::TapAndHoldGesture:
        break;
    case Qt::PanGesture:
        break;
    case Qt::PinchGesture:
        pinchGesture(static_cast<QPinchGesture*>(gesture));
        break;
    case Qt::SwipeGesture:
        swipeGesture(static_cast<QSwipeGesture*>(gesture));
        break;
    default:
        break;
    }
    return true;
}

/**
 * @bug This is not smooth.
 */
void Scene::pinchGesture (QPinchGesture* pinch)
{
    if (pinch->changeFlags().testFlag(QPinchGesture::ScaleFactorChanged)) {
        qreal scale = pinch->scaleFactor();
        d->camera->setDistance(d->camera->distance() / scale);
    }
}

void Scene::swipeGesture (QSwipeGesture* swipe)
{
    Q_UNUSED(swipe);
}

void Scene::drawScene ()
{
}

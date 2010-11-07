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

#include "defs.h"

#include "Scene.moc"

#include "ShaderProgram.h"
#include "OrbitalCamera.h"
#include "Shell.h"

#include "ui/Playlist.h"

#include <QTimer>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QIcon>
#include <QDir>
#include <QWheelEvent>
#include <QGesture>
#include <QtConcurrentMap>
#include <QSplashScreen>

#include <QtFMOD/System.h>
#include <QtFMOD/Channel.h>
#include <QtFMOD/Sound.h>

#include <LinearMath/btVector3.h>

#include <btBulletDynamicsCommon.h>

#define SKY_TEX_MAX_WIDTH 2048

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

#define splashShowMessage(msg)                                              \
    do {                                                                    \
        qDebug() << msg;                                                    \
        d->splash->showMessage(msg,                                         \
                               Qt::AlignLeft|Qt::AlignBottom, Qt::cyan);    \
        qApp->processEvents();                                              \
    } while (0)

QPointer<Scene> scene;

struct Scene::Private
{
    /// Flags helps to call processEvents without causing other problems
    bool glInitializing;

    QSettings* settings;

    QTimer* timer;
    QtFMOD::System* fsys;
    QHash<QString, QSharedPointer<QtFMOD::Sound> > sounds;

    QSharedPointer<QtFMOD::Channel> channel;
    QSharedPointer<QtFMOD::Sound> sound;

    int spectrumLength;
    FMOD_DSP_FFT_WINDOW spectrumWindowType;
    QVector<float> spectrumNew[2];      ///< values before smoothing
    QVector<float> spectrum[2];         ///< values after smoothing

    GLuint cubeMapTex;
    GLuint starTex;

    OrbitalCamera* camera;

    ShaderProgram* skyShader;
    ShaderProgram* debugNormalsShader;
    ShaderProgram* fyreworksShader;
    QHash<QString, QPointer<ShaderProgram> > shaders;

    QTime time;
    qreal dt;

    QSplashScreen* splash;
    Playlist* playlist;

    btDynamicsWorld* dynamicsWorld;
    btBroadphaseInterface* broadphaseInterface;
    btConstraintSolver* constraintSolver;
    btCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;

    Private (Scene* q) :
        glInitializing(false),
        settings(new QSettings(q)),
        timer(new QTimer(q)),
        fsys(new QtFMOD::System(q)),
        spectrumLength(256),
        spectrumWindowType(FMOD_DSP_FFT_WINDOW_RECT),
        cubeMapTex(0),
        starTex(0),
        camera(new OrbitalCamera(q)),
        skyShader(new ShaderProgram(q)),
        debugNormalsShader(new ShaderProgram(q)),
        fyreworksShader(new ShaderProgram(q)),
        dt(0.01),
        splash(new QSplashScreen(QPixmap(":media/images/splash.png"))),
        playlist(new Playlist), // leak

        dynamicsWorld(NULL),
        broadphaseInterface(NULL),
        constraintSolver(NULL),
        collisionConfiguration(NULL),
        dispatcher(NULL)
    {
        timer->setObjectName("timer");
        fsys->setObjectName("fsys");

        spectrumNew[0].resize(spectrumLength);
        spectrumNew[1].resize(spectrumLength);
        spectrum[0].resize(spectrumLength);
        spectrum[1].resize(spectrumLength);

        shaders.insert("sky", skyShader);
        shaders.insert("debugNormals", debugNormalsShader);
        shaders.insert("fyreworks", fyreworksShader);

        QMetaObject::connectSlotsByName(q);
    }
};

Scene::Scene (QWidget* parent) :
    QGLWidget(parent),
    d(new Private(this))
{
    Q_ASSERT(scene.isNull());
    scene = this;

    d->splash->show();
    qApp->processEvents();

    splashShowMessage("initializing...");

    connect(d->timer, SIGNAL(timeout()), d->fsys, SLOT(update()));

    initSound();
    initPhysics();
    initGui();

    grabGesture(Qt::TapGesture);
    grabGesture(Qt::TapAndHoldGesture);
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
    grabGesture(Qt::SwipeGesture);

    // start simulation
    d->timer->start(16);
}

Scene::~Scene ()
{
}

ShaderProgram* Scene::shader (const QString& name) const
{
    return d->shaders[name];
}

btDynamicsWorld* Scene::dynamicsWorld () const
{
    return d->dynamicsWorld;
}

Camera* Scene::camera () const
{
    return d->camera;
}

QtFMOD::System* Scene::soundSystem () const
{
    return d->fsys;
}

QSharedPointer<QtFMOD::Sound> Scene::sound (const QString& name) const
{
    return d->sounds[name];
}

void Scene::initSound ()
{
    splashShowMessage("sound...");

    // sound system
    d->fsys->init(32);
    fsysCheck();

    d->fsys->set3DNumListeners(1);
    d->fsys->set3DSettings(1.0f, 1.0f, 0.3f);

    // sound effects
    QSharedPointer<QtFMOD::Sound> sound (
        d->fsys->createSound(":media/sfx/explosion0.oga", FMOD_3D)
        );
    fsysCheck();
    d->sounds.insert("explosion", sound);
    sound->set3DMinMaxDistance(150, 600);
}

void Scene::initPhysics ()
{
    splashShowMessage("physics...");

    d->collisionConfiguration = new btDefaultCollisionConfiguration();
    Q_CHECK_PTR(d->collisionConfiguration);

    d->dispatcher = new btCollisionDispatcher(d->collisionConfiguration);
    Q_CHECK_PTR(d->dispatcher);

    d->broadphaseInterface = new btDbvtBroadphase();
    Q_CHECK_PTR(d->broadphaseInterface);

    d->constraintSolver = new btSequentialImpulseConstraintSolver;
    Q_CHECK_PTR(d->constraintSolver);

    d->dynamicsWorld = new btDiscreteDynamicsWorld(
        d->dispatcher,
        d->broadphaseInterface,
        d->constraintSolver,
        d->collisionConfiguration
        );
    Q_CHECK_PTR(d->dynamicsWorld);

    d->dynamicsWorld->setInternalTickCallback(internalTickCallback, this);

    d->dynamicsWorld->setGravity(btVector3(0, -9.806, 0));
}

//#include <QDockWidget>
void Scene::initGui ()
{
    splashShowMessage("gui...");

    //QDockWidget* playlistDock = new QDockWidget("Playlist", this);
    //playlistDock->show();
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

    d->playlist->show();
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
    // when initializeGL takes too long, and processEvents is called,
    // initializeGL gets called again
    if (d->glInitializing) {
        return;
    } else {
        d->glInitializing = true;
    }

    splashShowMessage("graphics...");

    qglClearColor("black");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    splashShowMessage("sky...");

    /// @todo remove the hard coded value
    loadCubeMap(QDir("Bridge.cubemap"));

    makeStarTex(64);

    initFinal();
    d->glInitializing = false;
}

void Scene::initFinal ()
{
    // remove splash screen
    d->splash->finish(this);
    d->splash->deleteLater();
    metaObject()->invokeMethod(d->playlist, "update", Qt::QueuedConnection);
}

void Scene::resizeGL (int w, int h)
{
    glViewport(0, 0, w, h);
}

void Scene::paintGL ()
{
    if (d->glInitializing) {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, qreal(width())/height(), 1.0, 1000.0);
    d->camera->setMaxDistance(1000.0);
    d->camera->setFocus(btVector3(0, 50, 0));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    d->camera->invoke();
    d->fsys->set3DListenerAttributes(
        0,
        d->camera->position(),
        d->camera->velocity(),
        d->camera->forward(),
        d->camera->up()
        );

    drawSceneShells();
    drawSky();
    drawSceneClusters();
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

void Scene::checkTags ()
{
    if (!d->sound) {
        return;
    }
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

void Scene::analyzeSound ()
{
    if (!d->channel) {
        return;
    }
    qreal sum[2];
    for (int chan = 0; chan < 2; chan++) {
        sum[chan] = 0.0;
        for (int i = 0; i < d->spectrum[chan].size(); i++) {
            sum[chan] += d->spectrum[chan][i];
        }
    }
    if (((1.0 / (sum[0] + sum[1])) * randf(1000)) < 1.0) {
        launch();
    }
}

void Scene::launch ()
{
    Shell* shell = new Shell(this);
    connect(this, SIGNAL(drawShells()), shell, SLOT(draw()));
    connect(this, SIGNAL(update(qreal)), shell, SLOT(update(qreal)));
}

void Scene::on_timer_timeout ()
{
    qreal realDt = 0.001 * d->time.restart();
    expMovAvg(d->dt, realDt, 30);

    checkTags();
    analyzeSound();

    // calling stepSimulation eventually leads to internalTickCallback,
    // which eventually emits update signal
    d->dynamicsWorld->stepSimulation(d->dt);

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

void Scene::makeStarTex (int maxWidth)
{
    Q_ASSERT(d->starTex == 0);
    glGenTextures(1, &d->starTex);
    glBindTexture(GL_TEXTURE_2D, d->starTex);

    QVector<float> img;
    img.reserve(maxWidth * maxWidth);
    for (int width = maxWidth, level = 0; width > 0; width>>=1, level++) {
        img.resize(0);
        qreal radius = width >> 1;
        btVector3 center (radius, radius, 0.0f);
        for (int y = 0; y < width; y++) {
            for (int x = 0; x < width; x++) {
                btVector3 p (x, y, 0.0f);
                qreal distance = p.distance(center);
                qreal zo = distance / radius;
                img << (zo > 1.0 ? 0.0 : 1.0 - zo);
            }
        }

        if (qFuzzyCompare(radius, 0.0)) {
            img[0] = 1.0;
        }

        glTexImage2D(GL_TEXTURE_2D, level, GL_LUMINANCE,
                     width, width, 0, GL_LUMINANCE, GL_FLOAT,
                     img.data());

#if 0
        for (int y = 0; y < width; y++) {
            for (int x = 0; x < width; x++) {
                printf("%f ", img[x + y * width]);
            }
            printf("\n");
        }
        printf("\n");
#endif

    }
}

struct ImageLoader
{
    typedef QList<QImage> result_type;

    result_type operator() (const QString& path)
    {
        QImage img (path);

        /// @bug verify image flipping
#if 1
        img = QGLWidget::convertToGLFormat(img);
#else
        img = img.rgbSwapped();
#endif

        QList<QImage> images;

        for (int width = SKY_TEX_MAX_WIDTH; width > 0; width >>= 1)
        {
            img = img.scaled(width, width,
                             Qt::IgnoreAspectRatio,
                             Qt::SmoothTransformation
                            );
            images << img;
        }

        return images;
    }
};

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

    d->debugNormalsShader->addShaderFromSourceFile(
        CG_GL_VERTEX, ":media/shaders/debugNormals.cg", "main_vp");
    d->debugNormalsShader->addShaderFromSourceFile(
        CG_GL_FRAGMENT, ":media/shaders/debugNormals.cg", "main_fp");
    d->debugNormalsShader->link();
    if (d->debugNormalsShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->debugNormalsShader->errorString();
    }

    d->fyreworksShader->addShaderFromSourceFile(
        CG_GL_VERTEX, ":media/shaders/fyreworks.cg", "main_vp");
    d->fyreworksShader->addShaderFromSourceFile(
        CG_GL_FRAGMENT, ":media/shaders/fyreworks.cg", "main_fp");
    d->fyreworksShader->link();
    if (d->fyreworksShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->fyreworksShader->errorString();
    }

    /**
     * @bug Check this.  Using convertToGLFormat(), which flips in addition to rgb swapping.  Then, flipping x and y in the shader fragment's cube texture lookup.  And finally, swapped neg and pos y images.
     */

    // prep images
    QTime t0;
    t0.start();

    QStringList imagePaths;
    imagePaths
        << path.filePath("posx.jpg") << path.filePath("negx.jpg")
        << path.filePath("negy.jpg") << path.filePath("posy.jpg")
        << path.filePath("posz.jpg") << path.filePath("negz.jpg")
        ;
    QFuture<QList<QImage> > future =
        QtConcurrent::mapped(imagePaths, ImageLoader());
    future.waitForFinished();
    QList<QList<QImage> > images (future.results());

    qDebug() << t0.restart() << "ms loading sky";

    // upload
    int pos = 0;
    int level;
    foreach (const QList<QImage>& imageLevels, images) {
        level = 0;
        foreach (const QImage& img, imageLevels) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + pos, level, GL_RGBA,
                         img.width(), img.height(),
                         0, GL_RGBA, GL_UNSIGNED_BYTE,
                         img.bits()
                        );
            level++;
        }
        pos++;
    }

    qDebug() << t0.elapsed() << "ms uploading sky";
}

/**
 * @todo Replace with a vertex buffer.
 */
void Scene::drawSky ()
{
    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, d->cubeMapTex);

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
        gluSphere(quadric, 1000.0, 16, 16);
        gluDeleteQuadric(quadric);

        glEndList();
    }
    glPopMatrix();

    d->skyShader->release();

    if (d->skyShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->skyShader->errorString();
    }

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

void Scene::drawSceneShells ()
{
    d->debugNormalsShader->bind();
    emit drawShells();
    d->debugNormalsShader->release();

    if (d->debugNormalsShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->debugNormalsShader->errorString();
    }
}

void Scene::drawSceneClusters ()
{
    glPushAttrib(GL_ENABLE_BIT);

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_POINT_SPRITE);
    glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive blending

    glDepthMask(GL_FALSE);

    glBindTexture(GL_TEXTURE_2D, d->starTex);

    d->fyreworksShader->bind();
    emit drawClusters();
    d->fyreworksShader->release();

    glDepthMask(GL_TRUE);

    glPopAttrib();

    if (d->fyreworksShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->fyreworksShader->errorString();
    }
}

/**
 * Bullet callback.
 */
void Scene::internalTickCallback (btDynamicsWorld* world, btScalar timeStep)
{
    Scene* scene = static_cast<Scene*>(world->getWorldUserInfo());
    emit scene->update(timeStep);
}

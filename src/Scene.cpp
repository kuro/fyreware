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

#include "SoundEngine.h"

#include "ShaderProgram.h"
#include "OrbitalCamera.h"
#include "Shell.h"
#include "FPSGraph.h"

#include "scripting.h"

#include <QGLWidget>
#include <QTimer>
#include <QCoreApplication>
#include <QDebug>
#include <QIcon>
#include <QDir>
#include <QWheelEvent>
#include <QGesture>
#include <QtConcurrentMap>
#include <QGLContext>
#include <QUrl>
#include <QScriptEngine>

#include <LinearMath/btVector3.h>

#include <btBulletDynamicsCommon.h>

#include <QtFMOD/System.h>

#define SKY_TEX_MAX_WIDTH 1024

#define SPECTRUM_HEIGHT 1

#define glCheck()                                                           \
    do {                                                                    \
        GLuint gl_error = glGetError();                                     \
        if (gl_error != GL_NO_ERROR) {                                      \
            qFatal("OpenGL Error [%s:%d]: %s\n",                            \
                   Q_FUNC_INFO, __LINE__, gluErrorString(gl_error));        \
        }                                                                   \
    } while (0)

#define sendStatusMessage(msg)                                              \
    do {                                                                    \
        qDebug() << msg;                                                    \
        emit statusMessage(msg, Qt::AlignLeft, Qt::cyan);                   \
        qApp->processEvents();                                              \
    } while (0)

QPointer<Scene> scene;

struct Scene::Private
{
    QTimer* timer;

    GLuint cubeMapTex;
    GLuint starTex;

    OrbitalCamera* camera;

    ShaderProgram* skyShader;
    ShaderProgram* debugNormalsShader;
    ShaderProgram* fyreworksShader;
    QHash<QString, QPointer<ShaderProgram> > shaders;

    QTime time;
    qreal dt;

    FPSGraph* fpsGraph;

    QScriptEngine* scriptEngine;
    QHash<QString, QScriptProgram> shellPrograms;
    QScriptProgram analyzerProgram;

    btDynamicsWorld* dynamicsWorld;
    btBroadphaseInterface* broadphaseInterface;
    btConstraintSolver* constraintSolver;
    btCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;

    Private (Scene* q) :
        timer(new QTimer(q)),
        cubeMapTex(0),
        starTex(0),
        camera(new OrbitalCamera(q)),
        skyShader(new ShaderProgram(q)),
        debugNormalsShader(new ShaderProgram(q)),
        fyreworksShader(new ShaderProgram(q)),
        dt(0.016),
        fpsGraph(new FPSGraph(QSizeF(120 * 1.5, 60), 120, 60, q)),
        scriptEngine(new QScriptEngine(q)),

        dynamicsWorld(NULL),
        broadphaseInterface(NULL),
        constraintSolver(NULL),
        collisionConfiguration(NULL),
        dispatcher(NULL)
    {
        timer->setObjectName("timer");

        shaders.insert("sky", skyShader);
        shaders.insert("debugNormals", debugNormalsShader);
        shaders.insert("fyreworks", fyreworksShader);

        QMetaObject::connectSlotsByName(q);
    }
};

Scene::Scene (QObject* parent) :
    QGraphicsScene(parent),
    d(new Private(this))
{
    Q_ASSERT(scene.isNull());
    scene = this;
}

Scene::~Scene ()
{
}

void Scene::start ()
{
    sendStatusMessage("initializing...");

    initSound();
    initPhysics();
    initScripting();
    initGraphics();

    //grabGesture(Qt::TapGesture);
    //grabGesture(Qt::TapAndHoldGesture);
    //grabGesture(Qt::PanGesture);
    //grabGesture(Qt::PinchGesture);
    //grabGesture(Qt::SwipeGesture);

    // start simulation
    d->time.start();
    d->timer->start(16);
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

qreal Scene::dt () const
{
    return d->dt;
}

QScriptEngine* Scene::scriptEngine () const
{
    return d->scriptEngine;
}

QHash<QString, QScriptProgram> Scene::shellPrograms () const
{
    return d->shellPrograms;
}

void Scene::initSound ()
{
    sendStatusMessage("sound...");

    Q_ASSERT(soundEngine);
    soundEngine->initialize();
}

void Scene::initPhysics ()
{
    sendStatusMessage("physics...");

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

    d->dynamicsWorld->setGravity(btVector3(0.f, -9.806f, 0.f));
}

static
QScriptProgram readScript (const QString& fileName)
{
    qDebug() << "loading" << fileName;
    QFile dev (fileName);
    if (!dev.open(QIODevice::ReadOnly)) {
        qWarning() << Q_FUNC_INFO << dev.errorString();
        return QScriptProgram();
    }
    QScriptProgram program (dev.readAll(), dev.fileName());
    dev.close();
    return program;
}

QScriptProgram Scene::analyzerProgram () const
{
    return d->analyzerProgram;
}

void Scene::initScripting ()
{
    sendStatusMessage("scripting...");

    QDir::addSearchPath("scripts", "scripts");

    // analyzer
    d->analyzerProgram = readScript("scripts:default.analyzer");
    if (d->analyzerProgram.isNull()) {
        qFatal("failed to load default.analyzer");
    }

    // shells
    QDir dir ("scripts:");
    dir.setNameFilters(QStringList()<<"*.shell");
    QStringList shells (dir.entryList());
    showit(shells);
    foreach (const QString& fileName, dir.entryList()) {
        QScriptProgram program = readScript(dir.filePath(fileName));
        if (!program.isNull()) {
            d->shellPrograms.insert(QFileInfo(fileName).baseName(), program);
        }
    }
}

#if 0
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

    //d->control->show();
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
#endif

static
void loadShader (
    ShaderProgram* shader,
    const QString& file,
    const QString& ventry,
    const QString& fentry
    )
{
    shader->addShaderFromSourceFile(CG_GL_VERTEX, file, ventry);
    shader->addShaderFromSourceFile(CG_GL_FRAGMENT, file, fentry);
    shader->link();
    if (shader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << shader->errorString();
    }
}

/**
 * @warning This has to wait until OpenGL is ready.
 */
void Scene::initGraphics ()
{
    sendStatusMessage("graphics...");

    glCheck();

    // sky
    sendStatusMessage("sky...");

    QDir dir (".");
    dir.setNameFilters(QStringList()<<"*.cubemap");
    QStringList cubemaps (dir.entryList());
    if (!cubemaps.isEmpty()) {
        QString cubemap (cubemaps[randi(cubemaps.size())]);
        qDebug() << "loading cubemap" << cubemap;
        loadCubeMap(QDir(cubemap));
    }
    loadShader(d->skyShader, ":media/shaders/sky.cg",
               "main_vp", "main_fp");

    // fyreworks
    makeStarTex(64);
    loadShader(d->fyreworksShader, ":media/shaders/fyreworks.cg",
               "main_vp", "main_fp");

    // shells
    loadShader(d->debugNormalsShader, ":media/shaders/debugNormals.cg",
               "main_vp", "main_fp");

    glCheck();
}

void Scene::drawBackground (QPainter* painter, const QRectF&)
{
    if (painter->paintEngine()->type() != QPaintEngine::OpenGL2) {
        return;
    }

    Q_ASSERT(QGLContext::currentContext()->isValid());

    painter->beginNativePainting();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, qreal(width())/height(), 0.01, 1000.0);
    d->camera->setMaxDistance(1000.0);
    d->camera->setFocus(btVector3(0, 50, 0));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    draw();

    painter->endNativePainting();
}

void Scene::draw ()
{
    Q_ASSERT(soundEngine);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    d->camera->invoke();
    soundEngine->soundSystem()->set3DListenerAttributes(
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
    d->fpsGraph->draw();

    glCheck();
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
    expMovAvg(d->dt, realDt, 120);
    d->fpsGraph->addSample(realDt, d->dt);

    QWidget* widget = qobject_cast<QWidget*>(parent());
    if (widget) {
        widget->setWindowTitle(tr("FyreWare (%0 fps)").arg(int(1.0/d->dt)));
    }

    // calling stepSimulation eventually leads to internalTickCallback,
    // which eventually emits update signal
    if (d->dynamicsWorld) {
        d->dynamicsWorld->stepSimulation(d->dt);
    }

    //updateGL();
    QGraphicsScene::update();
}

void Scene::drawSpectrum ()
{
    if (!soundEngine->isPlaying()) {
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, soundEngine->spectrumLength() * 2, 0, SPECTRUM_HEIGHT, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor3f(0, 1, 1);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < soundEngine->spectrumLength(); i++) {
        glVertex2f(i,
                   soundEngine->spectrum(0)[i]);
    }
    glEnd();

    glColor3f(1, 0, 0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i < soundEngine->spectrumLength(); i++) {
        /// @todo perfect horizontal alignment
        glVertex2f(soundEngine->spectrumLength() * 2 - i - 1,
                   soundEngine->spectrum(1)[i]);
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
    if (d->skyShader->isNull()) {
        return;
    }

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

#if 0
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
#endif

void Scene::drawSceneShells ()
{
    if (d->debugNormalsShader->isNull()) {
        return;
    }

    d->debugNormalsShader->bind();
    emit drawShells();
    d->debugNormalsShader->release();

    if (d->debugNormalsShader->error() != CG_NO_ERROR) {
        qCritical() << Q_FUNC_INFO << d->debugNormalsShader->errorString();
    }
}

void Scene::drawSceneClusters ()
{
    if (d->fyreworksShader->isNull()) {
        return;
    }

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

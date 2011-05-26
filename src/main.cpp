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
 * @file main.cpp
 * @brief main implementation
 */

#include "Scene.h"
#include "Playlist.h"
#include "SoundEngine.h"

#include "ui/ControlDialog.h"

#include <QApplication>
#include <QGLWidget>
#include <QSplashScreen>

#include <QImage>
#include <QPainter>
#include <QIcon>
#include <QDateTime>
#include <QDialog>
#include <QMainWindow>

#include "ui/GraphicsView.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QDebug>

static
QIcon makeFireIcon ()
{
    QSize size (64, 64);

    QImage img (size, QImage::Format_ARGB32);
    img.fill(0);

    QPainter painter;

    painter.begin(&img);

    QFont font (painter.font());
    font.setPixelSize(64);
    painter.setFont(font);

    QPen brush (painter.pen());
    brush.setColor(QColor(0xF4, 0x51, 0x2C));
    painter.setPen(brush);

    painter.drawText(QRect(QPoint(0, 0), size),
                     Qt::AlignCenter,
                     QString::fromLocal8Bit("火"));

    painter.end();

    return QPixmap::fromImage(img);
}

int main(int argc, char *argv[])
{
    QApplication app (argc, argv);

    // randomness
    qsrand(QDateTime::currentDateTime().toTime_t());
    qrand();

    app.setOrganizationName("MentalDistortion");
    app.setApplicationName("FyreWare");

    SoundEngine soundEngine;

    QApplication::setWindowIcon(makeFireIcon());

    QGLWidget* glWidget = new QGLWidget();
    glWidget->makeCurrent();

    Scene* scene = new Scene;

    QScopedPointer<GraphicsView> view (new GraphicsView(scene));
    scene->setParent(view.data());
    view->setViewport(glWidget);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->resize(900, 600);
    view->show();

    // splash image
    QSplashScreen* splash;

    QPixmap splashPixmap (":media/images/splash.png");

    splashPixmap = splashPixmap.scaled(
        splashPixmap.size().boundedTo(view->size()),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    splash = new QSplashScreen(splashPixmap);
    QGraphicsProxyWidget* splashItem = scene->addWidget(splash);

    QSize diff ((view->size() - splash->size()) / 2);
    splashItem->setPos(diff.width(), diff.height());

    QObject::connect(
        scene, SIGNAL(statusMessage(const QString&, int, const QColor&)),
        splash, SLOT(showMessage(const QString&, int, const QColor&)));

    qApp->processEvents();  // make sure window is visible
    scene->start();
    splash->deleteLater();

    ControlDialog* control = new ControlDialog(view.data());

#if 0
    // display control dialog as an external window
    control->show();
#else
    // display control dialog within the scene
    QGraphicsProxyWidget* controlItem
        = scene->addWidget(control, Qt::Window);
    // sometimes, the title bar gets hidden, which leaves me no way to move it
    controlItem->setPos(
        view->size().width() - control->size().width(),
        view->size().height() - control->size().height()
        );
#endif


    foreach (QGraphicsItem* item, scene->items()) {
        qDebug() << item;
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    return app.exec();
}

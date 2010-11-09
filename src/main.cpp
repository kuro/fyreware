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

#include "ui/Playlist.h"
#include "ui/Player.h"

#include <QApplication>
#include <QGLWidget>

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

    QApplication::setWindowIcon(makeFireIcon());


    GraphicsView* graphicsView = new GraphicsView;

    QGLWidget* glWidget = new QGLWidget();
    glWidget->makeCurrent();
    graphicsView->setViewport(glWidget);

    graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    graphicsView->resize(900, 600);

    Scene scene;
    graphicsView->setScene(&scene);
    graphicsView->show();

    QDialog* control = new QDialog(
        NULL, Qt::CustomizeWindowHint|Qt::WindowTitleHint
        );
    control->setSizeGripEnabled(true);
    control->setWindowOpacity(0.8);
    control->setWindowTitle("Control");
    control->setLayout(new QVBoxLayout);
    Playlist* playlist (new Playlist(control));
    QMetaObject::invokeMethod(playlist, "update", Qt::QueuedConnection);
    Player* player (new Player(control));
    control->layout()->addWidget(player);
    control->layout()->addWidget(playlist);

    QGraphicsProxyWidget* tmp = scene.addWidget(control, Qt::Window);


    foreach (QGraphicsItem* item, scene.items()) {
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }

    return app.exec();
}

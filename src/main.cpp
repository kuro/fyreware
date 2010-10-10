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

#include <QApplication>

#include <QImage>
#include <QPainter>
#include <QIcon>

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

    app.setOrganizationName("MentalDistortion");
    app.setApplicationName("FyreWare");

    QApplication::setWindowIcon(makeFireIcon());

    Scene scene;
    scene.show();

    return app.exec();
}

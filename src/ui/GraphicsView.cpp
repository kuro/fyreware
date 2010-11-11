
/**
 * @file GraphicsView.cpp
 * @brief GraphicsView implementation
 */

#include "GraphicsView.moc"

#include "Scene.h"

#include <QResizeEvent>

GraphicsView::GraphicsView (QGraphicsScene* scene) :
    QGraphicsView(scene)
{
    setWindowTitle(tr("Fyreware"));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

GraphicsView::~GraphicsView ()
{
}

void GraphicsView::resizeEvent (QResizeEvent* evt)
{
    if (scene()) {
        scene()->setSceneRect(QRect(QPoint(0, 0), evt->size()));
    }
    QGraphicsView::resizeEvent(evt);
}

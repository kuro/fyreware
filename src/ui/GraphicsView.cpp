
/**
 * @file GraphicsView.cpp
 * @brief GraphicsView implementation
 */

#include "GraphicsView.moc"

#include <QResizeEvent>

GraphicsView::GraphicsView () :
    QGraphicsView()
{
    setWindowTitle(tr("Fyreware"));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

void GraphicsView::resizeEvent (QResizeEvent* evt)
{
    if (scene()) {
        scene()->setSceneRect(QRect(QPoint(0, 0), evt->size()));
    }
    QGraphicsView::resizeEvent(evt);
}

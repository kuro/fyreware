
/**
 * @file GraphicsView.cpp
 * @brief GraphicsView implementation
 */

#include "GraphicsView.moc"

#include "Scene.h"
#include "OrbitalCamera.h"

#include <QResizeEvent>
#include <QDebug>

struct GraphicsView::Private
{
    QPoint lastPos;

    Private (GraphicsView* q)
    {
    }
};

GraphicsView::GraphicsView (QGraphicsScene* scene) :
    QGraphicsView(scene),
    d(new Private(this))
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

void GraphicsView::mousePressEvent (QMouseEvent* evt)
{
    if (itemAt(evt->pos()) && d->lastPos == QPoint()) {
        evt->ignore();
        QGraphicsView::mousePressEvent(evt);
    } else {
        d->lastPos = evt->pos();
        evt->accept();
    }
}

void GraphicsView::mouseMoveEvent (QMouseEvent* evt)
{
    if (d->lastPos == QPoint()) {
        evt->ignore();
        QGraphicsView::mouseMoveEvent(evt);
        return;
    }

    QPointF delta = evt->pos() - d->lastPos;
    delta *= ::scene->dt();

    OrbitalCamera* ocam = qobject_cast<OrbitalCamera*>(::scene->camera());
    if (ocam) {
        ocam->razimuth()  +=  delta.x();
        ocam->raltitude() += -delta.y();
    }

    d->lastPos = evt->pos();
    evt->accept();
}

void GraphicsView::mouseReleaseEvent (QMouseEvent* evt)
{
    if (d->lastPos == QPoint()) {
        evt->ignore();
        QGraphicsView::mouseReleaseEvent(evt);
    } else {
        d->lastPos = QPoint();
        evt->accept();
    }
}

/**
 * @todo Find a better way to support multi-touch trackpads
 */
void GraphicsView::wheelEvent (QWheelEvent* evt)
{
    if (itemAt(evt->pos())) {
        evt->ignore();
        QGraphicsView::wheelEvent(evt);
        return;
    }

    OrbitalCamera* ocam = qobject_cast<OrbitalCamera*>(::scene->camera());
    Q_ASSERT(ocam);

#if Q_OS_MAC
//    qreal delta;
//    switch (evt->orientation()) {
//    case Qt::Horizontal:
//        delta = -0.001 * evt->delta();
//        d->camera->setAzimuth(d->camera->azimuth() + delta);
//        break;
//    case Qt::Vertical:
//        delta = 0.001 * evt->delta();
//        d->camera->setAltitude(d->camera->altitude() + delta);
//        break;
//    default:
//        break;
//    }
#else
    switch (evt->orientation()) {
    case Qt::Vertical:
        ocam->rdistance() += evt->delta() * ::scene->dt() * 10;
        break;
    default:
        break;
    }
#endif

    evt->accept();
}

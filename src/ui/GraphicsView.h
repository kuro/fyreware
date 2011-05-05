
/**
 * @file GraphicsView.h
 * @brief GraphicsView definition
 */

#pragma once

#include <QGraphicsView>

class GraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    GraphicsView (QGraphicsScene* scene);
    virtual ~GraphicsView ();

    void mousePressEvent (QMouseEvent* evt);
    void mouseMoveEvent (QMouseEvent* evt);
    void mouseReleaseEvent (QMouseEvent* evt);
    void wheelEvent (QWheelEvent* evt);

protected:
    void resizeEvent (QResizeEvent* evt);

    void showEvent (QShowEvent* evt);
    void closeEvent (QCloseEvent* evt);

private:
    struct Private;
    QScopedPointer<Private> d;
};

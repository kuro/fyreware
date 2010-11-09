
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
    GraphicsView ();

protected:
    void resizeEvent (QResizeEvent* evt);
};

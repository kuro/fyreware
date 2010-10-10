
/**
 * @file Scene.h
 * @brief Scene definition
 */

#pragma once

#include <QGLWidget>

class Scene : public QGLWidget
{
    Q_OBJECT

public:
    Scene (QWidget* parent = NULL);
    virtual ~Scene ();

private:
    void loadSong (const QString& fileName);

    void initializeGL ();
    void resizeGL (int w, int h);
    void paintGL ();

    void drawSpectrum ();

    void showEvent (QShowEvent* evt);
    void closeEvent (QHideEvent* evt);

private slots:
    void on_timer_timeout ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

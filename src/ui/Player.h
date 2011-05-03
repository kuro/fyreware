
/**
 * @file Player.h
 * @brief Player definition
 */

#pragma once

#include <QWidget>

#include "ui_Player.h"

class Player : public QWidget, Ui::Player
{
    Q_OBJECT

public:
    Player (QWidget* parent = NULL);
    virtual ~Player ();

private:
    void timerEvent (QTimerEvent* evt);

private slots:
    void on_timeSlider_sliderMoved (int value);
    void on_volumeSlider_sliderMoved (int value);
    void on_timeSlider_sliderPressed ();
    void on_volumeSlider_sliderPressed ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

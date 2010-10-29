
/**
 * @file Playlist.h
 * @brief Playlist definition
 */

#pragma once

#include <QWidget>
#include "ui_Playlist.h"

class Playlist : public QWidget, Ui::Playlist
{
    Q_OBJECT

public:
    Playlist (QWidget* parent = NULL);
    virtual ~Playlist ();

    /**
     * @name drag and drop
     */
    //@{
    void dragEnterEvent (QDragEnterEvent* evt);
    void dragMoveEvent (QDragMoveEvent* evt);
    void dropEvent (QDropEvent* evt);
    //@}

private:
    struct Private;
    QScopedPointer<Private> d;
};

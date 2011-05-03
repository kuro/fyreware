
/**
 * @file PlaylistWidget.h
 * @brief PlaylistWidget definition
 */

#pragma once

#include <QWidget>

#include "ui_Playlist.h"

class PlaylistWidget : public QWidget, Ui::Playlist
{
    Q_OBJECT

public:
    PlaylistWidget (QWidget* parent = NULL);
    virtual ~PlaylistWidget ();

    Q_INVOKABLE void update ();

    /**
     * @name drag and drop
     */
    //@{
    void dragEnterEvent (QDragEnterEvent* evt);
    void dragMoveEvent (QDragMoveEvent* evt);
    void dropEvent (QDropEvent* evt);
    //@}

private slots:
    void playlist_inserted (int idx);

protected:
    void initDb ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

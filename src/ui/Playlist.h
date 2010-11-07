
/**
 * @file Playlist.h
 * @brief Playlist definition
 */

#pragma once

#include <QWidget>
#include "ui_Playlist.h"

namespace QtFMOD
{
class System;
}

class QSqlDatabase;

class Playlist : public QWidget, Ui::Playlist
{
    Q_OBJECT

public:
    Playlist (QWidget* parent = NULL);
    virtual ~Playlist ();

    Q_INVOKABLE void update ();

    /**
     * @name drag and drop
     */
    //@{
    void dragEnterEvent (QDragEnterEvent* evt);
    void dragMoveEvent (QDragMoveEvent* evt);
    void dropEvent (QDropEvent* evt);
    //@}

protected:
    void initDb ();

    /**
     * @name Threaded file scanning.
     */
    //@{
    void scanDir (const QString& path);
    void scanFile (const QString& path, QSqlDatabase& db, QtFMOD::System* fsys);
    //@}

private:
    struct Private;
    QScopedPointer<Private> d;
};

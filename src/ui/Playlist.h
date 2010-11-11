
/**
 * @file Playlist.h
 * @brief Playlist definition
 */

#pragma once

#include <QWidget>
#include <QDateTime>
#include "ui_Playlist.h"
#include "SortedSet.h"

namespace QtFMOD
{
class System;
}

class QSqlDatabase;

class QAbstractItemModel;
class QReadWriteLock;

class Playlist : public QWidget, Ui::Playlist
{
    Q_OBJECT

public:
    Playlist (QWidget* parent = NULL);
    virtual ~Playlist ();

    Q_INVOKABLE void update ();

    QSqlDatabase& db () const;
    SortedSet<QUrl>& urls () const;

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

    friend class DirectoryScanner;
    QAbstractItemModel* model () const;

private:
    struct Private;
    QScopedPointer<Private> d;
};

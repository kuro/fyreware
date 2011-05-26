
/**
 * @file Playlist.h
 * @brief Playlist definition
 */

#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>

#include "SortedSet.h"

class Playlist : public QObject, public SortedSet<QUrl>
{
    Q_OBJECT

public:
    Playlist (QObject* parent = NULL);
    virtual ~Playlist ();

public slots:
    /**
     * A sorted insert.
     */
    void insert (QUrl url);

signals:
    void inserted (int index);

protected:
    void advance (int offset) const;
    void loadCurrentSong ();

private:
    struct Private;
    QScopedPointer<Private> d;
};

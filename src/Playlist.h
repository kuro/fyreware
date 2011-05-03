
/**
 * @file Playlist.h
 * @brief Playlist definition
 */

#pragma once

#include <QObject>
#include <QPointer>
#include <QUrl>

class Playlist : public QObject
{
    Q_OBJECT

public:
    Playlist (QObject* parent = NULL);
    virtual ~Playlist ();

    int size () const;

    QUrl at (int idx) const;

public slots:
    void playPause ();
    void next ();
    void prev ();

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

extern QPointer<Playlist> playlist;


/**
 * @file Playlist.cpp
 * @brief Playlist implementation
 */

#include "Playlist.moc"

#include "defs.h"
#include "SortedSet.h"
#include "Scene.h"

#include <QtFMOD/Channel.h>

#include <QUrl>
#include <QDebug>

using namespace QtFMOD;

QPointer<Playlist> playlist;

struct Playlist::Private
{
    SortedSet<QUrl> urls;
    int current;

    Private (Playlist* q) :
        current(-1)
    {
        Q_UNUSED(q);
    }
};

Playlist::Playlist (QObject* parent) :
    QObject(parent),
    d(new Private(this))
{
    Q_ASSERT(playlist.isNull());
    playlist = this;
}

Playlist::~Playlist ()
{
}

int Playlist::size () const
{
    return d->urls.size();
}

QUrl Playlist::at (int idx) const
{
    return d->urls[idx];
}

/**
 * A sorted insert.
 *
 * Triggers inserted(index)
 */
void Playlist::insert (QUrl url)
{
    d->urls << url;
    emit inserted(d->urls.indexOf(url));
}

void Playlist::advance (int offset) const
{
    // check
    if (d->urls.isEmpty()) {
        d->current = -1;
        return;
    }

    // advance
    d->current += offset;

    // check bounds
    if (d->current < 0) {
        d->current = d->urls.size() - 1;
    }
    if (d->current >= d->urls.size()) {
        d->current = 0;
    }
}

void Playlist::prev ()
{
    advance(-1);
    loadCurrentSong();
}

void Playlist::next ()
{
    advance(1);
    loadCurrentSong();
}

void Playlist::playPause ()
{
    QSharedPointer<Channel> channel (scene->streamChannel());
    if (channel) {
        if (channel->isPlaying()) {
            channel->setPaused(!channel->paused());
        } else {
            loadCurrentSong();
        }
    } else {
        loadCurrentSong();
    }
}

void Playlist::loadCurrentSong ()
{
    // sanity checks
    if (d->urls.isEmpty()) {
        return;
    }

    // ensure current index is valid
    advance(0);

    // get current url
    QUrl url (d->urls[d->current]);
    Q_ASSERT(url.isValid());

    // load
    scene->loadSong(url.toString());

    // register callback
    QSharedPointer<Channel> channel (scene->streamChannel());
    Q_ASSERT(channel);
    connect(channel.data(), SIGNAL(soundEnded()), SLOT(next()));
}

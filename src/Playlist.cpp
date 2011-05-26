
/**
 * @file Playlist.cpp
 * @brief Playlist implementation
 */

#include "Playlist.moc"

#include "SoundEngine.h"
#include "defs.h"
#include "Scene.h"

#include <QUrl>
#include <QDebug>

#include <QtFMOD/Channel.h>

using namespace QtFMOD;

struct Playlist::Private
{
    Private (Playlist* q) //:
    {
        Q_UNUSED(q);
    }
};

Playlist::Playlist (QObject* parent) :
    QObject(parent),
    d(new Private(this))
{
}

Playlist::~Playlist ()
{
}

/**
 * A sorted insert.
 *
 * Triggers inserted(index)
 */
void Playlist::insert (QUrl url)
{
    *this << url;
    emit inserted(indexOf(url));
}

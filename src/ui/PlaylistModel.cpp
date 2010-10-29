
/**
 * @file PlaylistModel.cpp
 * @brief PlaylistModel implementation
 */

#include "PlaylistModel.moc"
#include "Scene.h"

#include <QDebug>
#include <QUrl>

#include <QtFMOD/System.h>
#include <QtFMOD/Sound.h>
#include <QtFMOD/Tag.h>

enum
{
    Album,
    Title,
    Artist,
    NbColumns
};

struct PlaylistModel::Private
{
    QList<QUrl>& urls;
    QContiguousCache<QHash<QString, QVariant> > tagsCache;

    Private (QList<QUrl>& urls) :
        urls(urls),
        tagsCache(100)
    {
    }

    QHash<QString, QVariant> getTags (int row);
};

PlaylistModel::PlaylistModel (QList<QUrl>& urls, QObject* parent) :
    QAbstractTableModel(parent),
    d(new Private(urls))
{
}

PlaylistModel::~PlaylistModel ()
{
}

QVariant PlaylistModel::headerData (int section, Qt::Orientation orientation,
                                    int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Album:
            return "Album";
        case Title:
            return "Name";
        case Artist:
            return "Artist";
        }
    }
    return QVariant();
}

int PlaylistModel::rowCount (const QModelIndex& parent) const
{
    return d->urls.size();
}

int PlaylistModel::columnCount (const QModelIndex& parent) const
{
    return NbColumns;
}

QHash<QString, QVariant> PlaylistModel::Private::getTags (int row)
{
    const QUrl url (urls[row]);
    qDebug() << row << url;

    QtFMOD::System* fsys = scene->soundSystem();
    Q_ASSERT(fsys);

    if (fsys->error() != FMOD_OK) {
        qWarning() << Q_FUNC_INFO << fsys->errorString();
    }

    QString fname;
    if (url.scheme() == "file") {
        fname = url.path();
    } else {
        fname = url.toString();
    }

    QHash<QString, QVariant> retval;

    QSharedPointer<QtFMOD::Sound> sound (fsys->createStream(fname, FMOD_3D));
    if (fsys->error() != FMOD_OK) {
        qWarning() << Q_FUNC_INFO << fsys->errorString();
        Q_ASSERT(sound.isNull());
    } else {
        Q_ASSERT(!sound.isNull());
        QHashIterator<QString, QtFMOD::Tag> it (sound->tags());
        while (it.hasNext()) {
            it.next();
            retval.insert(it.key(), it.value().value());
        }
    }

    return retval;
}

static inline
QVariant findTag (const QHash<QString, QVariant>& tags,
                  const QStringList& names)
{
    foreach (const QString& tag, names) {
        if (tags.contains(tag)) {
            return tags[tag];
        }
    }
    return QVariant();
}

QVariant PlaylistModel::data (const QModelIndex& index, int role) const
{
    // update cache as necessary
    while (index.row() > d->tagsCache.lastIndex()) {
        d->tagsCache.append(d->getTags(d->tagsCache.lastIndex() + 1));
    }
    while (index.row() < d->tagsCache.firstIndex()) {
        d->tagsCache.append(d->getTags(d->tagsCache.firstIndex() - 1));
    }

    // generate data
    const QHash<QString, QVariant> tags (d->tagsCache.at(index.row()));
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case Album:
            return findTag(tags,
                           QStringList() << "ALBUM" << "TAL" << "icy-name");
        case Title:
            return findTag(tags,
                           QStringList() << "TITLE" << "TT2" << "icy-url");
        case Artist:
            return findTag(tags,
                           QStringList() << "ARTIST" << "TP1");
        }
    }

    return QVariant();
}

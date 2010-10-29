
/**
 * @file PlaylistModel.cpp
 * @brief PlaylistModel implementation
 */

#include "defs.h"

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
    int lookAhead, halfLookAhead;
    int count;

    QStringList albumTags;
    QStringList titleTags;
    QStringList artistTags;


    Private (QList<QUrl>& urls) :
        urls(urls),
        tagsCache(1000),
        lookAhead(16),
        halfLookAhead(lookAhead >> 1),
        count(0)
    {
        albumTags  << "ALBUM" << "TAL" << "icy-name";
        titleTags  << "TITLE" << "TT2" << "icy-url";
        artistTags << "ARTIST" << "TP1";
    }

    QHash<QString, QVariant> fetchRow (int row);
    void cacheRows (int from, int to);
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
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    switch (orientation) {
    case Qt::Horizontal:
        switch (section) {
        case Album:
            return "Album";
        case Title:
            return "Name";
        case Artist:
            return "Artist";
        }
        break;
    case Qt::Vertical:
        return section;
    default:
        break;
    }

    return QVariant();
}

int PlaylistModel::rowCount (const QModelIndex& parent) const
{
    return d->count;
}

int PlaylistModel::columnCount (const QModelIndex& parent) const
{
    return NbColumns;
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

QHash<QString, QVariant> PlaylistModel::Private::fetchRow (int row)
{
    Q_ASSERT(row < urls.size());
    const QUrl url (urls[row]);
    qDebug() << row << url;

    QtFMOD::System* fsys = scene->soundSystem();
    Q_ASSERT(fsys);

    if (fsys->error() != FMOD_OK) {
        qWarning() << Q_FUNC_INFO << __LINE__ << fsys->errorString();
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
        //qWarning() << Q_FUNC_INFO << __LINE__ << fsys->errorString();
        Q_ASSERT(sound->isNull());
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

/**
 * @warning inclusive
 */
void PlaylistModel::Private::cacheRows (int from, int to)
{

    for (int i = from; i <= to; ++i) {
        tagsCache.insert(i, fetchRow(i));
    }
}

QVariant PlaylistModel::data (const QModelIndex& index, int role) const
{
    int row = index.row();

    if (row >= d->count) {
        qWarning() << __LINE__ << "index is too large";
    }

    // update cache as necessary
#if 0
    while (row > d->tagsCache.lastIndex()) {
        d->tagsCache.append(d->fetchRow(d->tagsCache.lastIndex() + 1));
    }
    while (row < d->tagsCache.firstIndex()) {
        d->tagsCache.prepend(d->fetchRow(d->tagsCache.firstIndex() - 1));
    }
#else
    if (row > d->tagsCache.lastIndex()) {
        if (row - d->tagsCache.lastIndex() > d->lookAhead) {
            d->cacheRows(row - d->halfLookAhead,
                         qMin(d->count - 1, row + d->halfLookAhead));
        } else {
            while (row > d->tagsCache.lastIndex()) {
                d->tagsCache.append(d->fetchRow(d->tagsCache.lastIndex()+1));
            }
        }
    } else if (row < d->tagsCache.firstIndex()) {
        if (d->tagsCache.firstIndex() - row > d->lookAhead) {
            d->cacheRows(qMax(0, row - d->halfLookAhead),
                         row + d->halfLookAhead);
        } else {
            while (row < d->tagsCache.firstIndex()) {
                d->tagsCache.prepend(d->fetchRow(d->tagsCache.firstIndex()-1));
            }
        }
    }
#endif

    // generate data
    const QHash<QString, QVariant> tags (d->tagsCache.at(row));
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case Album:
            return findTag(tags, d->albumTags);
        case Title:
            return findTag(tags, d->titleTags);
        case Artist:
            return findTag(tags, d->artistTags);
        }
    }

    return QVariant();
}

bool PlaylistModel::insertRows (int row, int count, const QModelIndex& parent)
{
    if (count != 1) {
        qWarning() << "refusing to insert more than 1";
        return false;
    }
    if (row != d->count) {
        qWarning() << "refusing to perform an op other than append";
        return false;
    }
    beginInsertRows(parent, row, row + count - 1);
    d->count += count;
    endInsertRows();
    return true;
}

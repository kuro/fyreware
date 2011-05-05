
/**
 * @file PlaylistModel.cpp
 * @brief PlaylistModel implementation
 */

#include "defs.h"

#include "PlaylistModel.moc"

#include "Playlist.h"
#include "Scene.h"
#include "SortedSet.h"

#include <QUrl>
#include <QDebug>
#include <QPointer>
#include <QMutexLocker>

#include <QSqlDatabase>
#include <QSqlQuery>

enum
{
    Album,
    Title,
    Artist,
    NbColumns
};

struct StreamInfo
{
    QString album;
    QString title;
    QString artist;
};

struct PlaylistModel::Private
{
    QContiguousCache<StreamInfo> tagsCache;
    int lookAhead, halfLookAhead;

    Private () :
        tagsCache(10000),
        lookAhead(100),
        halfLookAhead(lookAhead >> 1)
    {
    }

    StreamInfo fetchRow (int row);
    void cacheRows (int from, int to);
};

PlaylistModel::PlaylistModel (QObject* parent) :
    QAbstractTableModel(parent),
    d(new Private())
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
    Q_UNUSED(parent);
    return playlist->size();
}

int PlaylistModel::columnCount (const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return NbColumns;
}

StreamInfo PlaylistModel::Private::fetchRow (int row)
{
    StreamInfo info;

    QSqlQuery q;
    q.prepare("select album, title, artist from streams where url = :url");
    q.bindValue(":url", playlist->at(row).toString());
    if (!q.exec() || !q.next()) {
        return info;
    }
    info.album  = q.value(0).toString();
    info.title  = q.value(1).toString();
    info.artist = q.value(2).toString();

    return info;
}

/**
 * @warning inclusive
 */
void PlaylistModel::Private::cacheRows (int from, int to)
{
    for (int i = from; i <= to; i++) {
        tagsCache.insert(i, fetchRow(i));
    }
}

QVariant PlaylistModel::data (const QModelIndex& index, int role) const
{
    int row = index.row();

//    if (row >= playlist->size()) {
//        qWarning() << __LINE__ << "index is too large";
//    }

    // update cache as necessary
    if (row > d->tagsCache.lastIndex()) {
        if (row - d->tagsCache.lastIndex() > d->lookAhead) {
            d->cacheRows(row - d->halfLookAhead,
                         qMin(playlist->size() - 1, row + d->halfLookAhead));
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

    // generate data
    const StreamInfo& info (d->tagsCache.at(row));
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case Album:
            return info.album;
        case Title:
            return info.title;
        case Artist:
            return info.artist;
        }
    case FilterRole:
        QString str;
        str += info.album;
        str += info.title;
        str += info.artist;
        return str;
    }

    return QVariant();
}

bool PlaylistModel::insertRows (int row, int count, const QModelIndex& parent)
{
    Q_ASSERT(count == 1);

    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();

    return true;
}

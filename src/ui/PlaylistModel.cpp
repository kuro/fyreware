
/**
 * @file PlaylistModel.cpp
 * @brief PlaylistModel implementation
 */

#include "PlaylistModel.moc"

#include <QDebug>
#include <QUrl>

struct PlaylistModel::Private
{
    QList<QUrl>& urls;

    Private (QList<QUrl>& urls) :
        urls(urls)
    {
    }
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
    QVariant retval;
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            retval = "Url";
            break;
        }
    }
    return retval;
}

int PlaylistModel::rowCount (const QModelIndex& parent) const
{
    return d->urls.size();
}

int PlaylistModel::columnCount (const QModelIndex& parent) const
{
    return 1;
}

QVariant PlaylistModel::data (const QModelIndex& index, int role) const
{
    qDebug() << index;
    const QUrl& url = d->urls[index.row()];
    switch (role) {
    case Qt::DisplayRole:
        return url.toString();
    }
    return QVariant();
}

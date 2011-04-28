
/**
 * @file Playlist.cpp
 * @brief Playlist implementation
 *
 * @todo Load in background, notifying of changes.
 * @todo Use a database backend to avoid hitting fmod heavily.
 */

#include "Playlist.moc"

#include "SortedSet.h"
#include "defs.h"
#include "Scene.h"
#include "PlaylistModel.h"
#include "DirectoryScanner.h"

#include <QtFMOD/Channel.h>

#include <QDebug>
#include <QDragEnterEvent>
#include <QUrl>
#include <QDir>
#include <QDesktopServices>
#include <QSortFilterProxyModel>
#include <QThreadPool>
#include <QDateTime>

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

using namespace QtFMOD;

QPointer<Playlist> playlist;

struct Playlist::Private
{
    SortedSet<QUrl> urls;
    int current;

    QSqlDatabase db;
    PlaylistModel* model;
    QSortFilterProxyModel* proxyModel;

    Private (Playlist* q) :
        current(-1),
        model(new PlaylistModel(urls, q)),
        proxyModel(new QSortFilterProxyModel(q))
    {
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRole(FilterRole);
        proxyModel->setSourceModel(model);
    }
};

Playlist::Playlist (QWidget* parent) :
    QWidget(parent),
    d(new Private(this))
{
    Q_ASSERT(!playlist);
    playlist = this;

    setupUi(this);

    initDb();

    tableView->setModel(d->proxyModel);

    connect(filterLineEdit, SIGNAL(textChanged(const QString&)),
            d->proxyModel, SLOT(setFilterWildcard(const QString&)));
}

Playlist::~Playlist ()
{
}

QSqlDatabase& Playlist::db () const
{
    return d->db;
}

SortedSet<QUrl>& Playlist::urls () const
{
    return d->urls;
}

QUrl Playlist::current () const
{
    if (d->current < 0) {
        d->current = 0;
    }
    if (d->current >= d->urls.size()) {
        return QUrl();
    }
    return d->urls[d->current];
}

QUrl Playlist::advance (int offset) const
{
    d->current += offset;
    if (d->current < 0) {
        d->current = d->urls.size() - 1;
    }
    if (d->current >= d->urls.size()) {
        d->current = 0;
    }
    return current();
}

QAbstractItemModel* Playlist::model () const
{
    return d->model;
}

void Playlist::update ()
{
    QSqlQuery q;
    q.prepare("select url from streams order by url");
    if (q.exec()) {
        while (q.next()) {
            QUrl url (q.value(0).toString());
            d->urls << url;
            d->model->insertRow(d->urls.indexOf(url));
        }
    }

    QThreadPool::globalInstance()->start(
        new DirectoryScanner(
            QDesktopServices::storageLocation(
                QDesktopServices::MusicLocation), this));

}

void Playlist::initDb ()
{
    QDir dataDir (
        QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    QDir::home().mkpath(dataDir.path());

#if 1
    d->db = QSqlDatabase::addDatabase("QSQLITE");
    d->db.setDatabaseName(dataDir.filePath("fyreware.db"));
#else
    d->db = QSqlDatabase::addDatabase("QPSQL");
    d->db.setHostName("sakura");
    d->db.setDatabaseName("fyreware");
    d->db.setUserName("blanton");
    d->db.setPassword("8eg4me2");
#endif

    if (!d->db.open()) {
        qFatal("failed to open database");
    }

    QSqlRecord record (d->db.record("streams"));
    if (record.isEmpty()) {
        QSqlQuery q;
        q.exec(
            "create table streams ("
            "    url varchar primary key,"
            "    album varchar,"
            "    title varchar,"
            "    artist varchar,"
            "    createdAt timestamp,"
            "    updatedAt timestamp"
            ")"
            );
    }
}

void Playlist::dragEnterEvent (QDragEnterEvent* evt)
{
    if (evt->mimeData()->hasFormat("text/uri-list")) {
        if (evt->source() == this) {
            evt->setDropAction(Qt::MoveAction);
            evt->accept();
        } else {
            evt->acceptProposedAction();
        }
    } else {
        evt->ignore();
    }
}

void Playlist::dragMoveEvent (QDragMoveEvent* evt)
{
    if (evt->mimeData()->hasFormat("text/uri-list")) {
        if (evt->source() == this) {
            evt->setDropAction(Qt::MoveAction);
            evt->accept();
        } else {
            evt->acceptProposedAction();
        }
    } else {
        evt->ignore();
    }
}

void Playlist::dropEvent (QDropEvent* evt)
{
    if (evt->mimeData()->hasFormat("text/uri-list")) {
        if (evt->source() == this) {
            evt->setDropAction(Qt::MoveAction);
            evt->accept();
        } else {
            evt->acceptProposedAction();
        }
    } else {
        evt->ignore();
    }
}

void Playlist::play ()
{
    scene->loadSong(current().toString());

    QSharedPointer<Channel> channel (scene->streamChannel());
    Q_ASSERT(channel);
    connect(channel.data(), SIGNAL(soundEnded()), SLOT(next()));
}

void Playlist::prev ()
{
    scene->loadSong(advance(-1).toString());

    QSharedPointer<Channel> channel (scene->streamChannel());
    Q_ASSERT(channel);
    connect(channel.data(), SIGNAL(soundEnded()), SLOT(next()));
}

void Playlist::next ()
{
    scene->loadSong(advance(1).toString());

    //QSharedPointer<Channel> channel (scene->streamChannel());
    //Q_ASSERT(channel);
    //connect(channel.data(), SIGNAL(soundEnded()), SLOT(next()));
}


/**
 * @file PlaylistWidget.cpp
 * @brief PlaylistWidget implementation
 *
 * @todo Load in background, notifying of changes.
 * @todo Use a database backend to avoid hitting fmod heavily.
 */

#include "PlaylistWidget.moc"

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

QPointer<PlaylistWidget> playlist;

struct PlaylistWidget::Private
{
    SortedSet<QUrl> urls;
    int current;

    QSqlDatabase db;
    PlaylistModel* model;
    QSortFilterProxyModel* proxyModel;

    Private (PlaylistWidget* q) :
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

PlaylistWidget::PlaylistWidget (QWidget* parent) :
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

PlaylistWidget::~PlaylistWidget ()
{
}

QSqlDatabase& PlaylistWidget::db () const
{
    return d->db;
}

SortedSet<QUrl>& PlaylistWidget::urls () const
{
    return d->urls;
}

QUrl PlaylistWidget::current () const
{
    if (d->current < 0) {
        d->current = 0;
    }
    if (d->current >= d->urls.size()) {
        return QUrl();
    }
    return d->urls[d->current];
}

QUrl PlaylistWidget::advance (int offset) const
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

QAbstractItemModel* PlaylistWidget::model () const
{
    return d->model;
}

void PlaylistWidget::update ()
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

void PlaylistWidget::initDb ()
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

void PlaylistWidget::dragEnterEvent (QDragEnterEvent* evt)
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

void PlaylistWidget::dragMoveEvent (QDragMoveEvent* evt)
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

void PlaylistWidget::dropEvent (QDropEvent* evt)
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

void PlaylistWidget::play ()
{
    scene->loadSong(current().toString());

    QSharedPointer<Channel> channel (scene->streamChannel());
    Q_ASSERT(channel);
    connect(channel.data(), SIGNAL(soundEnded()), SLOT(next()));
}

void PlaylistWidget::prev ()
{
    scene->loadSong(advance(-1).toString());

    QSharedPointer<Channel> channel (scene->streamChannel());
    Q_ASSERT(channel);
    connect(channel.data(), SIGNAL(soundEnded()), SLOT(next()));
}

void PlaylistWidget::next ()
{
    scene->loadSong(advance(1).toString());

    //QSharedPointer<Channel> channel (scene->streamChannel());
    //Q_ASSERT(channel);
    //connect(channel.data(), SIGNAL(soundEnded()), SLOT(next()));
}

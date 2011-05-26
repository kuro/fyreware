
/**
 * @file PlaylistWidget.cpp
 * @brief PlaylistWidget implementation
 *
 * @todo Load in background, notifying of changes.
 * @todo Use a database backend to avoid hitting fmod heavily.
 */

#include "PlaylistWidget.moc"

#include "Playlist.h"
#include "SoundEngine.h"
#include "defs.h"
#include "PlaylistModel.h"
#include "DirectoryScanner.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QDir>
#include <QDesktopServices>
#include <QSortFilterProxyModel>
#include <QThreadPool>

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

struct PlaylistWidget::Private
{
    QSqlDatabase db;
    PlaylistModel* model;
    QSortFilterProxyModel* proxyModel;

    Private (PlaylistWidget* q) :
        model(new PlaylistModel(q)),
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
    setupUi(this);

    initDb();

    tableView->setModel(d->proxyModel);

    connect(filterLineEdit, SIGNAL(textChanged(const QString&)),
            d->proxyModel, SLOT(setFilterWildcard(const QString&)));

    connect(soundEngine->playlist(), SIGNAL(inserted(int)),
            SLOT(playlist_inserted(int)));
}

PlaylistWidget::~PlaylistWidget ()
{
}

void PlaylistWidget::update ()
{
    QSqlQuery q;
    q.prepare("select url from streams order by url");
    if (q.exec()) {
        while (q.next()) {
            QUrl url (q.value(0).toString());
            soundEngine->playlist()->insert(url);
        }
    }

    QThreadPool::globalInstance()->start(
        new DirectoryScanner(
            QDesktopServices::storageLocation(
                QDesktopServices::MusicLocation), d->db));

}

void PlaylistWidget::playlist_inserted (int idx)
{
    d->model->insertRow(idx);
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

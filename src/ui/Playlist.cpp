
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

#include <QDebug>
#include <QDragEnterEvent>
#include <QUrl>
#include <QDirIterator>
#include <QDesktopServices>
#include <QDateTime>
#include <QSortFilterProxyModel>

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

#include <QtConcurrentRun>

#include <boost/bind.hpp>

#include <QtFMOD/System.h>
#include <QtFMOD/Sound.h>
#include <QtFMOD/Tag.h>

struct Playlist::Private
{
    SortedSet<QUrl> urls;

    QSqlDatabase db;
    PlaylistModel* model;
    QSortFilterProxyModel* proxyModel;
    QStringList nameFilters;

    QStringList albumTags;
    QStringList titleTags;
    QStringList artistTags;

    Private (Playlist* q) :
        db(QSqlDatabase::addDatabase("QSQLITE")),
        model(new PlaylistModel(urls, db, q)),
        proxyModel(new QSortFilterProxyModel(q))
    {
        nameFilters
            //<< "*.aif"
            //<< "*.aiff"
            << "*.mp3"
            //<< "*.wav"
            ;

        albumTags  << "ALBUM" << "TAL" << "icy-name";
        titleTags  << "TITLE" << "TT2" << "icy-url";
        artistTags << "ARTIST" << "TP1";

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
    setupUi(this);

    initDb();

    tableView->setModel(d->proxyModel);

    connect(filterLineEdit, SIGNAL(textChanged(const QString&)),
            d->proxyModel, SLOT(setFilterWildcard(const QString&)));
}

Playlist::~Playlist ()
{
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

    QtConcurrent::run(boost::bind(&Playlist::scanDir, this,
                                  QDesktopServices::storageLocation(
                                      QDesktopServices::MusicLocation)));
}

void Playlist::initDb ()
{
    QDir dataDir (
        QDesktopServices::storageLocation(QDesktopServices::DataLocation));
    QDir::home().mkpath(dataDir.path());

    d->db.setDatabaseName(dataDir.filePath("fyreware.db"));

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

/**
 * @warning Designed to run in separate thread.
 */
void Playlist::scanDir (const QString& path)
{
    QScopedPointer<QtFMOD::System> fsys (new QtFMOD::System);
    fsys->init(1);

    QString connectionName ("tmp%0");
    connectionName = connectionName.arg((quint64)QThread::currentThreadId());
    {
        QSqlDatabase db = QSqlDatabase::cloneDatabase(d->db, connectionName);
        if (!db.open()) {
            qFatal("failed to open database");
        }

        QDirIterator dit (path, d->nameFilters, QDir::Files,
                          QDirIterator::Subdirectories);
        while (dit.hasNext()) {
            dit.next();
            scanFile(dit.filePath(), db, fsys.data());
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
}

static inline
QVariant findTag (const QHash<QString, QtFMOD::Tag>& tags,
                  const QStringList& names)
{
    foreach (const QString& tag, names) {
        if (tags.contains(tag)) {
            return tags[tag].value();
        }
    }
    return QVariant();
}

/**
 * @warning Designed to run in separate thread.
 */
void Playlist::scanFile (
    const QString& path,
    QSqlDatabase& db,
    QtFMOD::System* fsys
    )
{
    QSqlQuery q (db);

    QUrl url (path);
    QFileInfo fileInfo (path);

    q.prepare("select updatedAt from streams where url = :url limit 1");
    q.bindValue(":url", url.toString());
    if (!q.exec()) {
        qWarning() << q.lastError();
        qWarning() << q.lastQuery();
        return;
    }

    enum Op {
        Skip,
        Insert,
        Update
    };

    Op op = Skip;

    if (q.size() == 0) {
        op = Insert;
    } else {
        q.first();
        QDateTime updatedAt (q.value(0).toDateTime());
        if (url.scheme().isEmpty()) {
            // assuming it is a file
            if (!fileInfo.exists()) {
                qWarning() << path << "does not exist";
                op = Skip;
            } else if (updatedAt <= fileInfo.lastModified()) {
                op = Skip;
            } else {
                op = Update;
            }
        } else {
            Q_ASSERT(url.scheme() == "http");
            op = Update;
        }
    }

    if (op == Skip) {
        return;
    }

    QSharedPointer<QtFMOD::Sound> sound (
        fsys->createStream(path, FMOD_OPENONLY));

    if (fsys->error() != FMOD_OK) {
        qWarning() << "fmod failed to open" << path;
        Q_ASSERT(sound->isNull());
    } else {
        Q_ASSERT(!sound.isNull());

        QHash<QString, QtFMOD::Tag> tags (sound->tags());

        switch (op) {
        case Insert: {
            q.prepare(
                "insert into streams "
                "(url, album, title, artist, createdAt, updatedAt) "
                "values "
                "(:url, :album, :title, :artist, :createdAt, :updatedAt)"
                );
            q.bindValue(":createdAt", QDateTime::currentDateTime());
            break;
        }
        case Update: {
            q.prepare(
                "update streams set "
                "album = :album "
                "title = :title "
                "artist = :artist "
                "updatedAt = :updatedAt "
                "where url = :url"
                );
            break;
        }
        default: {
            qFatal("invalid op");
            break;
        }
        }

        q.bindValue(":url"   , url.toString());
        q.bindValue(":album" , findTag(tags, d->albumTags));
        q.bindValue(":title" , findTag(tags, d->titleTags));
        q.bindValue(":artist", findTag(tags, d->artistTags));
        q.bindValue(":updatedAt", fileInfo.lastModified());

        if (!q.exec()) {
            qWarning() << q.lastError();
            qWarning() << q.lastQuery();
        } else {
            if (op == Insert) {
                d->urls << url;
                d->model->insertRow(d->urls.indexOf(url));
            }
        }
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

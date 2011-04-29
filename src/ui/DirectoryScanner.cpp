
/**
 * @file DirectoryScanner.cpp
 * @brief DirectoryScanner implementation
 */

#include "DirectoryScanner.h"

#include "Playlist.h"
#include "defs.h"

#include <QtFMOD/System.h>
#include <QtFMOD/Sound.h>
#include <QtFMOD/Tag.h>

#include <QAbstractItemModel>
#include <QPointer>
#include <QDirIterator>
#include <QUrl>
#include <QDebug>
#include <QDateTime>

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlError>

struct DirectoryScanner::Private
{
    QPointer<Playlist> playlist;
    QString path;
    QStringList nameFilters;

    QStringList albumTags;
    QStringList titleTags;
    QStringList artistTags;

    QScopedPointer<QtFMOD::System> fsys;

    QSqlDatabase* db;

    Private (const QString& path, Playlist* playlist) :
        path(path),
        playlist(playlist),
        db(NULL)
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
    }
};

DirectoryScanner::DirectoryScanner (const QString& path, Playlist* playlist) :
    QRunnable(),
    d(new Private(path, playlist))
{
}

DirectoryScanner::~DirectoryScanner ()
{
}

void DirectoryScanner::run ()
{
    d->fsys.reset(new QtFMOD::System);
    d->fsys->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
    d->fsys->init(1);

    QString connectionName ("DirectoryScanner%0");
    connectionName = connectionName.arg((quint64)this);
    {
        QSqlDatabase db (
            QSqlDatabase::cloneDatabase(d->playlist->db(), connectionName));
        if (!db.open()) {
            qCritical() << db.lastError();
            return;
        }
        d->db = &db;
        QDirIterator dit (d->path, d->nameFilters, QDir::Files,
                          QDirIterator::Subdirectories);
        while (dit.hasNext()) {
            dit.next();
            if (!db.isOpen() || !d->playlist) {
                break;
            }
            scanFile(dit.filePath());
        }
        db.close();
    }
    QSqlDatabase::removeDatabase(connectionName);
    d->db = NULL;
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
void DirectoryScanner::scanFile (const QString& path)
{
    enum Op {
        Skip,
        Insert,
        Update
    };

    Op op = Skip;

    QUrl url (path);
    QFileInfo fileInfo (path);

    {
        QSqlQuery q (*d->db);
        q.prepare("select updatedAt from streams where url = :url limit 1");
        q.bindValue(":url", url.toString());
        if (!q.exec()) {
            qWarning() << Q_FUNC_INFO << __LINE__ << q.lastError();
            qWarning() << Q_FUNC_INFO << __LINE__ << q.lastQuery();
            return;
        }

        if (!q.first()) {
            op = Insert;
        } else {
            if (fileInfo.exists()) {
                QDateTime updatedAt (q.value(0).toDateTime());
                if (updatedAt <= fileInfo.lastModified()) {
                    op = Skip;
                } else {
                    op = Update;
                }
            } else if (url.scheme() == "http") {
                op = Update;
            } else {
                qFatal("%s: oops", Q_FUNC_INFO);
            }
        }
    }

    if (op == Skip) {
        return;
    }

    QSharedPointer<QtFMOD::Sound> sound (
        d->fsys->createStream(path, FMOD_OPENONLY));

    if (d->fsys->error() != FMOD_OK) {
        qWarning() << "fmod failed to open" << path;
        Q_ASSERT(sound->isNull());
    } else {
        Q_ASSERT(!sound.isNull());

        QHash<QString, QtFMOD::Tag> tags (sound->tags());

        QSqlQuery q (*d->db);
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
                "album = :album, "
                "title = :title, "
                "artist = :artist, "
                "updatedAt = :updatedAt "
                "where url = :url "
                );
            break;
        }
        default: {
            qCritical("invalid op");
            qApp->quit();
            break;
        }
        }

        q.bindValue(":url"   , url.toString());
        q.bindValue(":album" , findTag(tags, d->albumTags));
        q.bindValue(":title" , findTag(tags, d->titleTags));
        q.bindValue(":artist", findTag(tags, d->artistTags));
        q.bindValue(":updatedAt", fileInfo.lastModified());

        if (!q.exec()) {
            qWarning() << q.executedQuery();
            qWarning() << Q_FUNC_INFO << __LINE__ << q.lastQuery();
            qWarning() << Q_FUNC_INFO << __LINE__ << q.lastError();
        } else {
            if (op == Insert && d->playlist) {
                d->playlist->urls() << url;
                d->playlist->model()->insertRow(
                    d->playlist->urls().indexOf(url));
            }
        }
    }
}

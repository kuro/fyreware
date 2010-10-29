
/**
 * @file Playlist.cpp
 * @brief Playlist implementation
 *
 * @todo Load in background, notifying of changes.
 * @todo Use a database backend to avoid hitting fmod heavily.
 */

#include "Playlist.moc"
#include "PlaylistModel.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QUrl>
#include <QDirIterator>
#include <QDesktopServices>
#include <QtConcurrentRun>

#include <boost/bind.hpp>

struct Playlist::Private
{
    QList<QUrl> urls;
    PlaylistModel* model;
    QStringList nameFilters;

    Private (Playlist* q) :
        model(new PlaylistModel(urls, q))
    {
        nameFilters
            //<< "*.aif"
            //<< "*.aiff"
            << "*.mp3"
            //<< "*.wav"
            ;
    }
};

Playlist::Playlist (QWidget* parent) :
    QWidget(parent),
    d(new Private(this))
{
    setupUi(this);
#ifdef Q_OS_MAC
    //setWindowFlags(windowFlags() | Qt::Drawer);
#endif
    tableView->setModel(d->model);

    addFile("http://server1.kawaii-radio.net:9000");
    addFile("http://knr128.keiichi.net");

    QtConcurrent::run(boost::bind(&Playlist::addDir, this,
                                  QDesktopServices::storageLocation(
                                      QDesktopServices::MusicLocation)));
}

Playlist::~Playlist ()
{
}

void Playlist::addDir (const QString& path)
{
    QDirIterator dit (path, d->nameFilters, QDir::Files,
                      QDirIterator::Subdirectories);
    while (dit.hasNext()) {
        dit.next();
        metaObject()->invokeMethod(this, "addFile",
                                   Q_ARG(QString, dit.filePath())
                                   );
    }
}

void Playlist::addFile (const QString& path)
{
    int row = d->urls.size();
    d->urls << path;
    d->model->insertRow(row);
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

        foreach (const QUrl& url, evt->mimeData()->urls()) {
            qDebug() << url;
            qDebug() << url.scheme();
        }

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

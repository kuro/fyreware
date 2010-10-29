
/**
 * @file Playlist.cpp
 * @brief Playlist implementation
 */

#include "Playlist.moc"
#include "PlaylistModel.h"

#include <QDebug>
#include <QDragEnterEvent>
#include <QUrl>

struct Playlist::Private
{
    QList<QUrl> urls;
    PlaylistModel* model;

    Private (Playlist* q) :
        model(new PlaylistModel(urls, q))
    {
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

    d->urls << QUrl("http://server1.kawaii-radio.net:9000");
    d->urls << QUrl("http://knr128.keiichi.net");
}

Playlist::~Playlist ()
{
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

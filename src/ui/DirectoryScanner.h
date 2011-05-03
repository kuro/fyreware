
/**
 * @file DirectoryScanner.h
 * @brief DirectoryScanner definition
 */

#pragma once

#include <QObject>
#include <QRunnable>
#include <QScopedPointer>
#include <QUrl>

class PlaylistWidget;

class QSqlDatabase;

class DirectoryScanner : public QObject, public QRunnable
{
    Q_OBJECT

public:
    DirectoryScanner (const QString& path, const QSqlDatabase& dbToClone);
    virtual ~DirectoryScanner ();

    virtual void run ();

signals:
    void found (QUrl url);

protected:
    void scanFile (const QString& path);

private:
    struct Private;
    QScopedPointer<Private> d;
};
